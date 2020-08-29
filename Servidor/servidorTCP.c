#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <pthread.h>
#include "imagen.h"

#define PUERTO 			5000	//Número de puerto asignado al servidor
#define COLA_CLIENTES 	5 		//Tamaño de la cola de espera para clientes
#define NUM_HILOS 4

unsigned char *imagenRGB, *imagenGray, *imagenSobel;
int dimension; 					// Dimensión de la matriz en este caso de Sobel
uint32_t width = 0, height = 0; // Variables globales para que los hilos sepan las dimensiones de la imagen

unsigned char *reservarMemoria(int nBytes);
void RGBToGray_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void GrayToRGB_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void *deteccion_bordes(void *nh);

int main(int argc, char const *argv[])
{
	int sockfd, cliente_sockfd;
	struct sockaddr_in direccion_servidor; //Estructura de la familia AF_INET, que almacena direccion
	bmpInfoHeader info;
	// << VARS FOR THREADS
	pthread_t tids[NUM_HILOS];
	int nhs[NUM_HILOS];
	int *hilo;
	register int nh;
 	// >>

	/*	
	*	AF_INET - Protocolo de internet IPV4
	*  htons - El ordenamiento de bytes en la red es siempre big-endian, por lo que
	*  en arquitecturas little-endian se deben revertir los bytes
	*  INADDR_ANY - Se elige cualquier interfaz de entrada
	*/	
  	memset (&direccion_servidor, 0, sizeof (direccion_servidor));	//se limpia la estructura con ceros
	direccion_servidor.sin_family 		= AF_INET;
	direccion_servidor.sin_port 		= htons(PUERTO);
	direccion_servidor.sin_addr.s_addr 	= INADDR_ANY;

	/*
	*	Creacion de las estructuras necesarias para el manejo de un socket
	*  SOCK_STREAM - Protocolo orientado a conexión
	*/
	printf("Creando Socket ....\n");
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  	{
		perror("Ocurrio un problema en la creacion del socket");
		exit(1);
	}
  
	/*
	*  bind - Se utiliza para unir un socket con una dirección de red determinada
	*/
	printf("Configurando socket ...\n");
	if(bind(sockfd, (struct sockaddr *) &direccion_servidor, sizeof(direccion_servidor)) < 0)
	{
	   	perror ("Ocurrio un problema al configurar el socket");
	   	exit(1);
	}
  
	/*
	*  listen - Marca al socket indicado por sockfd como un socket pasivo, esto es, como un socket
	*  que será usado para aceptar solicitudes de conexiones de entrada usando "accept"
	*  Habilita una cola asociada la socket para alojar peticiones de conector procedentes
	*  de los procesos clientes
	*/
	printf ("Estableciendo la aceptacion de clientes...\n");
	if(listen(sockfd, COLA_CLIENTES) < 0)
	{
		perror("Ocurrio un problema al crear la cola de aceptar peticiones de los clientes");
		exit(1);
	}
  
	/*
	*  accept - Aceptación de una conexión
	*  Selecciona un cliente de la cola de conexiones establecidas
	*  se crea un nuevo descriptor de socket para el manejo
	*  de la nueva conexion. Apartir de este punto se debe
	*  utilizar el nuevo descriptor de socket
	*  accept - ES BLOQUEANTE 
	*/
	printf ("En espera de peticiones de conexión ...\n");
	// RESERVANDO MEMORIA PARA LOS ARREGLOS QUE SE UTILIZAN POSTERIORMENTE
	imagenGray = reservarMemoria(info.width * info.height);
	imagenSobel = reservarMemoria(info.width * info.height);
	while(1)
	{
		// ACEPTANDO CONEXIONES
		if((cliente_sockfd = accept(sockfd, NULL, NULL)) < 0)
		{
			perror("Ocurrio algun problema al atender a un cliente");
			exit(1);
		}

		// TOMANDO LA CAPTURA CON LA CÁMARA DE LA RASP
		if(system("raspistill -n -t 500 -e bmp -w 640 -h 480 -o foto.bmp") == -1)
		{
			perror("Ocurrio algun problema al tomar la captura");
			exit(1);	
		}
		
		// ABRIENDO LA IMAGEN A ENVIAR AL CLIENTE
		printf("Abriendo imagen...\n");		
		imagenRGB = abrirBMP("foto.bmp", &info);
		displayInfo(&info); 

		// GUARDANDO DICHA IMAGEN EN ESCALA DE GRISES
		RGBToGray_v2(imagenRGB, imagenGray, info.width, info.height);
		
		// LIMPIANDO EL ARREGLO SOBEL
		for(int x = 0; x < width * height; x++)
			imagenSobel[x] = 0;

		// ASIGNANDO VLAORES A LAS VARIABLES GLOBALES QUE TRABAJAN CON HILOS
		dimension = 3;				// Dimensión del filtro(matriz) de Sobel
		width = info.width;			// Ancho de la imagen
		height = info.height;		// Alto de la imagen
		
		// LLAMANDO HILOS PARA APLICAR DETECCIÓN DE BORDES
		for(nh = 0; nh < NUM_HILOS; nh++)
		{
			nhs[nh] = nh;
			pthread_create(&tids[nh], NULL, deteccion_bordes, (void *) &nhs[nh]);
		}
		
		// ESPERANDO A QUE LOS HILOS TERMINEN SU EJECUCIÓN
		for(nh = 0; nh < NUM_HILOS; nh++)
		{
			pthread_join(tids[nh], (void **) &hilo);
			printf("Hilo %d terminado\n", *hilo);
		}

		// GUARDANDO LA IMAGEN SOBEL COMO RESPALDO
		GrayToRGB_v2(imagenRGB, imagenSobel, info.width, info.height);	
		guardarBMP("sobel.bmp", &info, imagenRGB);
	
		// ENVIANDO EL HEADER DE LA IMAGEN AL CLIENTE PARA QUE PUEDA PROCESARLA CONOCIENDO SU ALTO Y ANCHO
		printf("Enviando Informacion de la Imagen al cliente ...\n");
		if(write(cliente_sockfd, &info, sizeof(bmpInfoHeader)) < 0)
		{
			perror("Ocurrio un problema en el envio de la cabecera al cliente");
			exit(1);
		}

		// ENVIANDO LA IMAGEN AL CLIENTE
		printf("Enviando IMAGEN al cliente ...\n");
		if(write(cliente_sockfd, imagenSobel, sizeof(unsigned char) * info.width * info.height) < 0)
		{
			perror("Ocurrio un problema en el envio de la imagen al cliente");
			exit(1);
		}

		printf("Imagen enviada, cerrando comunicación con el cliente...\n\n");

		free(imagenRGB);
		// close(cliente_sockfd);
	}

	printf("Concluimos la ejecución de la aplicacion Servidor\n");

	close (sockfd);
	
	return 0;
}

// @FUNCION QUE APLICA DETECCIÓN DE BORDES A UNA IMAGEN USANDO EL GRADIENTE (OPERADOR DE SOBEL, K = 2)
void *deteccion_bordes(void *nh)
{
	// CALCULANDO EL BLOQUE QUE LE TOCA PROCESAR A ESTE HILO
	int core = *(int *)nh;
	int elemsBloque = (width * height) / NUM_HILOS;
	int inicioBloque = (core * elemsBloque);
	int finBloque = (inicioBloque + elemsBloque) / width;
	inicioBloque = inicioBloque / width;	

	// VARIABLES PARA TRABAJAR CON SOBEL
	register int x, y, xb, yb;								// Índices e índices offset (b)
	int convolucion1, convolucion2, indice, resultado;
	int gradiente_fila[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1}; // Gradientes de Sobel
	int gradiente_col[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};	// Gradientes de Sobel

	if(core == 3) // Cuando se trata del último bloque se resta la dimensión para no calcular fuera del límite de la imagen
		finBloque = finBloque - dimension;

	// RECORREMOS LA IMAGEN Y APLICAMOS SOBEL DE ACUERDO AL BLOQUE QUE LE TOCÓ A ESTE HILO
	for(y = inicioBloque; y < finBloque; y++)
		for(x = 0; x < width - dimension; x++)
		{
			// HACEMOS LA CONVOLUCION DEL FILTRO CON LA IMAGEN
			resultado = 0;
			for (yb = 0; yb < dimension; yb++)
				for (xb = 0; xb < dimension; xb++)
				{
					indice = (y + yb) * width + (x + xb);
					convolucion1 += gradiente_fila[yb * dimension + xb] * imagenGray[indice];
					convolucion2 += gradiente_col[yb * dimension + xb] * imagenGray[indice];
				}
			
			// GUARDAMOS EL RESULTADO
			convolucion1 = convolucion1 >> 2;
			convolucion2 = convolucion2 >> 2;
			resultado = (int) sqrt(convolucion1 * convolucion1 + convolucion2 * convolucion2);

			indice = (y + (dimension >> 1)) * width + (x + (dimension >> 1));
			imagenSobel[indice] = resultado;
		}

	pthread_exit(nh);
}

void GrayToRGB_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height)
{
	int indiceGray = 0;

	int nBytesImagen = width * height * 3;
	for(int i  = 0; i < nBytesImagen; i += 3)
	{
		imagenRGB[i] = imagenGray[indiceGray];
		imagenRGB[i + 1] = imagenGray[indiceGray];
		imagenRGB[i + 2] = imagenGray[indiceGray++];
	}
}

void RGBToGray_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height)
{
	unsigned char nivelGris;
	int indiceGray = 0;
	int nBytesImagen = width * height * 3;

	for(int i = 0; i < nBytesImagen; i += 3)
	{
		nivelGris = (imagenRGB[i] * 30 + imagenRGB[i + 1] * 59 + imagenRGB[i + 2] * 11) / 100;
		imagenGray[indiceGray++] = nivelGris;			
	}	
}

unsigned char *reservarMemoria(int nBytes)
{
	unsigned char *imagen;
	imagen = (unsigned char *) malloc(nBytes);

	if(imagen == NULL)
	{
		perror("Error: No se pudo asignar memoria");
		exit(EXIT_FAILURE);
	}

	return imagen;
}