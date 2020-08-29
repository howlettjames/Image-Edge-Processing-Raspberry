/* Referencia:
   https://poesiabinaria.net/2011/06/leyendo-archivos-de-imagen-en-formato-bmp-en-c/
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "imagen.h"

unsigned char *abrirBMP(char *filename, bmpInfoHeader *bInfoHeader)
{

  	FILE *f;
  	bmpFileHeader header;     /* cabecera */
  	unsigned char *imgdata;   /* datos de imagen */
 	uint16_t type;        /* 2 bytes identificativos */

  	f = fopen (filename, "r");
	if( f  == NULL )
	{
		perror("Error al abrir el archivo en lectura");
		exit(EXIT_FAILURE);
	}

  	/* Leemos los dos primeros bytes */
  	fread( &type, sizeof(uint16_t), 1, f );
  	if( type != 0x4D42 )        /* Comprobamos el formato */
    	{
		printf("Error en el formato de imagen, debe ser BMP de 24 bits");
      		fclose(f);
      		return NULL;
    	}

  	/* Leemos la cabecera de fichero completa */
  	fread( &header, sizeof(bmpFileHeader), 1, f );

  	/* Leemos la cabecera de información completa */
  	fread( bInfoHeader, sizeof(bmpInfoHeader), 1, f );

  	/* Reservamos memoria para la imagen, ¿cuánta?
     	Tanto como indique imgsize */
  	imgdata = (unsigned char *)malloc( bInfoHeader->imgsize );
	if( imgdata == NULL )
	{
		perror("Error al asignar memoria");
		exit(EXIT_FAILURE);
	}
  	/* Nos situamos en el sitio donde empiezan los datos de imagen,
   	nos lo indica el offset de la cabecera de fichero*/
  	fseek(f, header.offset, SEEK_SET);

  	/* Leemos los datos de imagen, tantos bytes como imgsize */
  	fread(imgdata, bInfoHeader->imgsize,1, f);

  	/* Cerramos el apuntador del archivo de la imagen*/
  	fclose(f);

  	/* Devolvemos la imagen */
  	return imgdata;
}

void guardarBMP( char *filename, bmpInfoHeader *info, unsigned char *imgdata )
{
	bmpFileHeader header;
  	FILE *f;
  	uint16_t type;

  	f = fopen(filename, "w+");
	if( f  == NULL )
	{
		perror("Error al abrir el archivo en escritura");
		exit(EXIT_FAILURE);
	}

  	header.size = info->imgsize + sizeof(bmpFileHeader) + sizeof(bmpInfoHeader);
  	/* header.resv1=0; */
  	/* header.resv2=1; */
  	/* El offset será el tamaño de las dos cabeceras + 2 (información de fichero)*/
  	header.offset = sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + 2;
  	/* Escribimos la identificación del archivo */
  	type = 0x4D42;
  	fwrite( &type, sizeof(type), 1, f );
  	/* Escribimos la cabecera de fichero */
  	fwrite(&header, sizeof(bmpFileHeader),1,f);
  	/* Escribimos la información básica de la imagen */
  	fwrite(info, sizeof(bmpInfoHeader),1,f);
  	/* Escribimos la imagen */
  	fwrite(imgdata, info->imgsize, 1, f);
  	fclose(f);
}

void displayInfo( bmpInfoHeader *info )
{
  	printf("Tamaño de la cabecera: %u\n", info->headersize);
  	printf("Anchura: %d\n", info->width);
  	printf("Altura: %d\n", info->height);
  	printf("Planos (1): %d\n", info->planes);
  	printf("Bits por pixel: %d\n", info->bpp);
  	printf("Compresión: %d\n", info->compress);
  	printf("Tamaño de datos de imagen: %u\n", info->imgsize);
  	printf("Resolucón horizontal: %u\n", info->bpmx);
  	printf("Resolucón vertical: %u\n", info->bpmy);
  	printf("Colores en paleta: %d\n", info->colors);
  	printf("Colores importantes: %d\n", info->imxtcolors);
}
