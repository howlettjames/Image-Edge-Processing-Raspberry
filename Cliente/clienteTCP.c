#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "imagen.h"

#define PUERTO 5000
// #define DIR_IP "127.0.0.1"
#define DIR_IP "192.168.100.213"

unsigned char *imagenRGB, *imagenGray;

unsigned char *reservarMemoria(int nBytes);
void RGBToGray_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void GrayToRGB_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
void recibirImagen(int sockfd, unsigned char *imagenGray, int longitud);

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in direccion_servidor;
	bmpInfoHeader info;

	/*	
	 *	AF_INET - Protocolo de internet IPV4
	 *  htons - El ordenamiento de bytes en la red es siempre big-endian, por lo que
	 *  en arquitecturas little-endian se deben revertir los bytes
	 */	
	memset(&direccion_servidor, 0, sizeof (direccion_servidor));
	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_port = htons(PUERTO);
	/*	
	 *	inet_pton - Convierte direcciones de texto IPv4 en forma binaria
	 */	
	if(inet_pton(AF_INET, DIR_IP, &direccion_servidor.sin_addr) <= 0)
	{
		perror("Ocurrio un error al momento de asignar la direccion IP");
		exit(1);
	}
	/*
	*  Creacion de las estructuras necesarias para el manejo de un socket
	*  SOCK_STREAM - Protocolo orientado a conexión
	*/
	printf("Creando Socket ....\n");
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Ocurrio un problema en la creacion del socket");
		exit(1);
	}
	/*
	 *	Inicia el establecimiento de una conexion mediante una apertura
	 *	activa con el servidor
	 *  connect - ES BLOQUEANTE
	 */
	printf("Estableciendo conexion ...\n");
	if(connect(sockfd, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0) 
	{
		perror ("Ocurrio un problema al establecer la conexion");
		exit(1);
	}

	// LEYENDO EL HEADER DE LA IMAGEN EN ESCALA DE GRISES A RECIBIR
	printf("Se realizo la conexion con el servidor, recibiendo informacion de la imagen\n");
	if(read(sockfd, &info, sizeof(bmpInfoHeader)) < 0)
	{
		perror ("Ocurrio algun problema al recibir los datos de cabecera de la imagen");
		exit(1);
	}

	// RESERVANDO LA MEMORIA NECESARIA Y RECIBIENDO LA IMAGEN
	imagenGray = reservarMemoria(info.width * info.height);
	recibirImagen(sockfd, imagenGray, info.width * info.height);

	// RESERVANDO LA MEMORIA NECESARIA PARA GUARDAR LA IMAGEN EN RGB Y GUARDÁNDOLA
	imagenRGB = reservarMemoria(info.width * info.height * 3);
	GrayToRGB_v2(imagenRGB, imagenGray, info.width, info.height); 
	guardarBMP("recv.bmp", &info, imagenRGB);  

	printf("Cerrando la aplicacion cliente\n");
	
	/*
	 *	Cierre de la conexion
	 */
	close(sockfd);
	free(imagenGray);
	free(imagenRGB);	

	return 0;
}

void recibirImagen(int sockfd, unsigned char *imagenGray, int longitud)
{
   int bytesFaltantes = longitud, bytesRcv;

   while(bytesFaltantes > 0)
   {
      bytesRcv = read(sockfd, imagenGray, bytesFaltantes);
      if(bytesRcv < 0)
      {
         perror ("Ocurrio algun problema al recibir datos del cliente");
         exit(1);
      }

      printf("Bytes leidos: %d\n", bytesRcv);
      bytesFaltantes -= bytesRcv;  
      imagenGray = imagenGray + bytesRcv;
   }
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