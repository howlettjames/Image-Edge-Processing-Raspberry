#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "imagen.h"

#define NUM_HILOS 4

unsigned char *imagenRGB, *imagenGray, *imagenGrayS;
unsigned char *filtro;
int factor = 0; // Factor es la suma de todos los componentes de la matriz
int dimension = 5; // Dimensión de la matriz
uint32_t width = 0, height = 0;
	
void RGBToGray_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
unsigned char *reservarMemoria(uint32_t width, uint32_t height);
void GrayToRGB_v2(unsigned char *imagenRGB, unsigned char *imagenGray, uint32_t width, uint32_t height);
unsigned char *calcularKernelGauss(int *factor, int dim, double desviacion);
void *aplicar_filtro(void *nh);
void *deteccion_bordes(void *nh);

int main(int argc, char const *argv[])
{
	bmpInfoHeader info;
	//VAR FOR THREADS
	pthread_t tids[NUM_HILOS];
	int nhs[NUM_HILOS];
	int *hilo;
	register int nh;

	printf("Abriendo imagen...\n");		
	imagenRGB = abrirBMP("head.bmp", &info);
	displayInfo(&info); 

	imagenGray = reservarMemoria(info.width, info.height);
	imagenGrayS = reservarMemoria(info.width, info.height);

	// LIMPIANDO EL ARREGLO S
	for(int x = 0; x < width * height; x++)
		imagenGrayS[x] = 0;

	RGBToGray_v2(imagenRGB, imagenGray, info.width, info.height);

//  +++++++++++++++++++++++ APLICANDO FILTRO GAUSSIANO
/*
	//ASIGNANDO VLAORES A LAS VARIABLES GLOBALES QUE TRABAJAN CON HILOS
	filtro = calcularKernelGauss(&factor, 5, 1.0);
	dimension = 5;
	width = info.width;
	height = info.height;
	
	// APLICANDO FILTRO GAUSIANO A LA IMAGEN POR MEDIO DE HILOS
	for(nh = 0; nh < NUM_HILOS; nh++)
	{
		nhs[nh] = nh;
		pthread_create(&tids[nh], NULL, aplicar_filtro, (void *) &nhs[nh]);
	}
*/
//  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//  +++++++++++++++++++++++ APLICANDO DETECCION DE BORDES

	//ASIGNANDO VALORES A LAS VARIABLES GLOBALES QUE TRABAJAN CON HILOS
	dimension = 3;
	width = info.width;
	height = info.height;
	
	// LLAMANDO HILOS PARA APLICAR DETECCIÓN DE BORDES
	for(nh = 0; nh < NUM_HILOS; nh++)
	{
		nhs[nh] = nh;
		pthread_create(&tids[nh], NULL, deteccion_bordes, (void *) &nhs[nh]);
	}

//  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// ESPERANDO A QUE LOS HILOS TERMINEN SU EJECUCIÓN
	for(nh = 0; nh < NUM_HILOS; nh++)
	{
		pthread_join(tids[nh], (void **) &hilo);
		printf("Hilo %d terminado\n", *hilo);
	}

	GrayToRGB_v2(imagenRGB, imagenGrayS, info.width, info.height);	
	guardarBMP("sobel.bmp", &info, imagenRGB);
	
	free(imagenRGB);	
	free(imagenGray);
	free(imagenGrayS);

	return 0;
}

// @FUNCION QUE APLICA DETECCIÓN DE BORDES A UNA IMAGEN USANDO EL GRADIENTE (OPERADOR DE SOBEL, K = 2)
void *deteccion_bordes(void *nh)
{
	int core = *(int *)nh;
	int elemsBloque = (width * height) / NUM_HILOS;
	int inicioBloque = (core * elemsBloque);
	int finBloque = (inicioBloque + elemsBloque) / width;
	inicioBloque = inicioBloque / width;	

	register int x, y, xb, yb;
	int convolucion1, convolucion2, indice, resultado;
	int gradiente_fila[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
	int gradiente_col[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

	if(core == 3) 
		finBloque = finBloque - dimension;

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
			imagenGrayS[indice] = resultado;
		}

	pthread_exit(nh);
}

// @FUNCION QUE APLICA UN FILTRO GAUSIANO A UNA IMAGEN
void *aplicar_filtro(void *nh)
{
	int core = *(int *)nh;
	int elemsBloque = (width * height) / NUM_HILOS;
	int inicioBloque = (core * elemsBloque);
	int finBloque = (inicioBloque + elemsBloque) / width;
	inicioBloque = inicioBloque / width;	

	register int x, y, xb, yb;
	int convolucion, indice;

	if(core == 3) 
		finBloque = finBloque - dimension;

	for(y = inicioBloque; y < finBloque; y++)
		for(x = 0; x < width - dimension; x++)
		{
			// HACEMOS LA CONVOLUCION DEL FILTRO CON LA IMAGEN
			convolucion = 0;
			for (yb = 0; yb < dimension; yb++)
				for (xb = 0; xb < dimension; xb++)
				{
					indice = (y + yb) * width + (x + xb);
					convolucion += filtro[yb * dimension + xb] * imagenGray[indice];
				}
			
			// GUARDAMOS EL RESULTADO	
			convolucion = convolucion / factor;
			indice = (y + (dimension >> 1)) * width + (x + (dimension >> 1));
			imagenGrayS[indice] = convolucion;
		}

	pthread_exit(nh);
}

unsigned char *calcularKernelGauss(int *factor, int dim, double desviacion)
{
	unsigned char *kernelGauss = (unsigned char *) malloc(dim * dim);
	int rango = dim >> 1;
	register int x, y, indice;
	double coef, valor_normalizacion;

	indice = 0;
	*factor = 0;
	for(y = 0; y < dim; y++)
		for(x = 0; x < dim; x++)
		{
			coef = expf( - (( ((x - rango) * (x - rango)) + ((y - rango) * (y - rango)) ) / (2 * desviacion * desviacion) ));
			if(!indice)
				valor_normalizacion = coef;

			kernelGauss[indice] = (unsigned char) (coef / valor_normalizacion);
			*factor += kernelGauss[indice++];
		}

	return kernelGauss;
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

unsigned char *reservarMemoria(uint32_t width, uint32_t height)
{
	unsigned char *imagen;
	imagen = (unsigned char *) malloc(width * height);

	if(imagen == NULL)
	{
		perror("Error: No se pudo asignar memoria");
		exit(EXIT_FAILURE);
	}

	return imagen;
}