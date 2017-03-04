#ifndef __pixutils_h
#define __pixutils_h
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "lodepng/lodepng.h"


#ifndef M_PI
 #define M_PI 3.14159265
#endif

#define degreesToRadians(degrees) (degrees * M_PI / 180.0)
#define MAXWIDTH 256

typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
}rgba;

typedef struct{
	unsigned char *image;
	unsigned int imageWidth;
	unsigned int imageHeight;
	rgba **pixArray_overlay;
}pixMap;

typedef struct{
	void (*function) (pixMap*,pixMap*,int,int,void*);
	void *data;	
}plugin;	

//public methods
pixMap* pixMap_read(char *inputFile); //return zero pointer if failure
int pixMap_write(pixMap *p, char *outputFile); //return 1 if failure
void pixMap_destroy (pixMap **p); //free memory and set p to zero

//new for assignment 4
int pixMap_write_bmp16(pixMap *p,char *filename); //return 1 if failure
plugin *plugin_parse(char *argv[],int *iptr); //parses the command line and returns a plugin pointer 
void pixMap_apply_plugin(pixMap *p,plugin *plug); //apply the plugin to the pixMap
void plugin_destroy(plugin **plug); //free plugin data
#endif
