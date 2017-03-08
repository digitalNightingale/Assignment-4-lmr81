#include "pixutils.h"
#include "bmp/bmp.h"

//private methods -> make static
static pixMap* pixMap_init();
static pixMap* pixMap_copy(pixMap *p);

static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);

static pixMap* pixMap_init(){
	pixMap *p=malloc(sizeof(pixMap));
	p->pixArray_overlay=0;
	return p;
}

void pixMap_destroy (pixMap **p){
	if(!p || !*p) return;
	pixMap *this_p=*p;
	if(this_p->pixArray_overlay)
	free(this_p->pixArray_overlay);
	if(this_p->image)free(this_p->image);
	free(this_p);
	this_p=0;
}

pixMap *pixMap_read(char *filename){
	pixMap *p=pixMap_init();
	int error;
	if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
		fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
		return 0;
	}
	p->pixArray_overlay=malloc(p->imageHeight*sizeof(rgba*));
	p->pixArray_overlay[0]=(rgba*) p->image;
	for(int i=1;i<p->imageHeight;i++){
		p->pixArray_overlay[i]=p->pixArray_overlay[i-1]+p->imageWidth;
	}
	return p;
}

int pixMap_write(pixMap *p,char *filename){
	int error=0;
	if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
		fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
		return 1;
	}
	return 0;
}

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init();
	*new=*p;
	new->image=malloc(new->imageHeight*new->imageWidth*sizeof(rgba));
	memcpy(new->image,p->image,p->imageHeight*p->imageWidth*sizeof(rgba));
	new->pixArray_overlay=malloc(new->imageHeight*sizeof(void*));
	new->pixArray_overlay[0]=(rgba*) new->image;
	for(int i=1;i<new->imageHeight;i++){
		new->pixArray_overlay[i]=new->pixArray_overlay[i-1]+new->imageWidth;
	}
	return new;
}

void pixMap_apply_plugin(pixMap *p,plugin *plug){
	pixMap *copy=pixMap_copy(p);
	for(int i=0;i<p->imageHeight;i++){
		for(int j=0;j<p->imageWidth;j++){
			plug->function(p,copy,i,j,plug->data);
		}
	}
	pixMap_destroy(&copy);
}

int pixMap_write_bmp16(pixMap *p,char *filename){
	//initialize the bmp type
	BMP16map *bmp16=BMP16map_init(p->imageHeight,p->imageWidth,0,5,6,5);
	if(!bmp16) return 1;
	char Rbits = 5;
	char Gbits = 6;
	char Bbits = 5;
	char Abits = 0;
	for(int i = 0; i < p->imageHeight; i++) {
		for(int j = 0; j < p->imageWidth; j++) {
			//need to flip one of the the row indices when copying
			int iPixel = p->imageHeight - i - 1;
			int jPixel = j;
			uint16_t pix16 = 0;
			uint16_t r16 = p->pixArray_overlay[iPixel][jPixel].r;
			uint16_t g16 = p->pixArray_overlay[iPixel][jPixel].g;
			uint16_t b16 = p->pixArray_overlay[iPixel][jPixel].b;
			uint16_t a16 = p->pixArray_overlay[iPixel][jPixel].a;
			// pushing bit to the left and right
			r16 = (r16 >> (8 - Rbits)) << 11;
			g16 = (g16 >> (8 - Gbits)) << 5;
			b16 = (b16 >> (8 - Bbits));
			a16 = (a16 >> (8 - Abits)) << 16;
			pix16 = r16 | g16 | b16 | a16;
			bmp16->pixArray[i][j] = pix16;
		}
	}
	BMP16map_write(bmp16,filename);
	BMP16map_destroy(&bmp16);
	return 0;
}

void plugin_destroy(plugin **plug){
	//free the allocated memory and set *plug to zero (NULL)
	if(!plug || !*plug) return;
	plugin *this_p = *plug;
	if(this_p->data) free(this_p->data);
	if(this_p) free(this_p);
	this_p = 0;
}

plugin *plugin_parse(char *argv[] ,int *iptr){
	//malloc new plugin
	plugin *new=malloc(sizeof(plugin));
	new->function=0;
	new->data=0;
	int i=*iptr;
	if(!strcmp(argv[i]+2,"rotate")){
		new->function = rotate;
		new->data = malloc(2 * sizeof(float));
		float *sc = (float*) new->data;
		i = *iptr;
		float theta = atof(argv[i + 1]);
		sc[0] = sin(degreesToRadians(-theta));
		sc[1] = cos(degreesToRadians(-theta));
		*iptr = i+2;
		return new;
	}
	if(!strcmp(argv[i]+2,"convolution")){
		new->function = convolution;
		new->data = malloc(9 * sizeof(int));
		int (*kernelArry)[3] = new->data;
		i = *iptr;
		int increment = 1;
		for(int j = 0; j < 3; j++) {
			for(int k = 0; k < 3; k++) {
				kernelArry[j][k] = atoi(argv[i + increment]);
				increment++;
			}
		}
		*iptr = i+10;
		return new;
	}
	if(!strcmp(argv[i]+2,"flipHorizontal")){
		new->function = flipHorizontal;
		i = *iptr;
		*iptr=i+2;
		return new;
	}
	if(!strcmp(argv[i]+2,"flipVertical")){
		new->function = flipVertical;
		i = *iptr;
		*iptr=i+2;
		return new;
	}
	return(0);
}

//define plugin functions

static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	float *sc = (float*) data;
	const float ox = p->imageWidth/2.0f;
	const float oy = p->imageHeight/2.0f;
	const float s = sc[0];
	const float c = sc[1];
	float rotx = c * (j - ox) - s * (oy - i) + ox;
	float roty = -(s * (j - ox) + c * (oy - i) - oy);
	int rotj = rotx + .5;
	int roti = roty + .5;
	if(roti >= 0 && roti < oldPixMap->imageHeight && rotj >= 0 && rotj < oldPixMap->imageWidth){
		memcpy(p->pixArray_overlay[i] + j, oldPixMap->pixArray_overlay[roti] + rotj, sizeof(rgba));
	} else {
		memset(p->pixArray_overlay[i] + j, 0 , sizeof(rgba));
	}
}

static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	//kernel is a 3x3 matrix of integers
	int (*kernel)[3] = data;
	int accumR = 0, accumG = 0, accumB = 0, accumA = 0;
	float kernSum = 0;
	for(int x = 0; x < 3; x++) {
		for(int y = 0; y < 3; y++) {
			kernSum += kernel[x][y];
		}
	}
	for (int kernRow = -1; kernRow < 2; kernRow++) { //for each kernel row in kernel
		for (int kernEle = -1; kernEle < 2; kernEle++) { //for each element in kernel row
			int krIndex = j + kernRow;
			int keIndex = i + kernEle;
			//if element position corresponding* to pixel position then
			if (krIndex < 0) krIndex = 0;
			else if(krIndex > oldPixMap->imageHeight - 1) krIndex = oldPixMap->imageHeight - 1;
			if (keIndex < 0) keIndex = 0;
			else if(keIndex > oldPixMap->imageWidth - 1) keIndex = oldPixMap->imageWidth - 1;
			//multiply element value corresponding* to pixel value
			//normalize by dividing by the sum of all the elements in the matrix
			// need absolute value for negative kernal and no normalizing.
			if (kernSum == 0) {
				accumR += fabs(oldPixMap->pixArray_overlay[keIndex][krIndex].r * (kernel[kernEle + 1][kernRow + 1]));
				accumG += fabs(oldPixMap->pixArray_overlay[keIndex][krIndex].g * (kernel[kernEle + 1][kernRow + 1]));
				accumB += fabs(oldPixMap->pixArray_overlay[keIndex][krIndex].b * (kernel[kernEle + 1][kernRow + 1]));
				accumA += fabs(oldPixMap->pixArray_overlay[keIndex][krIndex].a * (kernel[kernEle + 1][kernRow + 1]));
			} else {
			accumR += (oldPixMap->pixArray_overlay[keIndex][krIndex].r * (kernel[kernEle + 1][kernRow + 1])) / kernSum;
			accumG += (oldPixMap->pixArray_overlay[keIndex][krIndex].g * (kernel[kernEle + 1][kernRow + 1])) / kernSum;
			accumB += (oldPixMap->pixArray_overlay[keIndex][krIndex].b * (kernel[kernEle + 1][kernRow + 1])) / kernSum;
			accumA += (oldPixMap->pixArray_overlay[keIndex][krIndex].a * (kernel[kernEle + 1][kernRow + 1])) / kernSum;
			}
		}
	}
	// if the pixel is greater less than zero set to 0
	// if the pixel is greater than 255 set to 255
	if(accumR > 255) accumR = 255;
	else if (accumR < 0) accumR = 0;
	if(accumG > 255) accumG = 255;
	else if (accumG < 0) accumG = 0;
	if(accumB > 255) accumB = 255;
	else if (accumB < 0) accumB = 0;
	if(accumA > 255) accumA = 255;
	else if (accumA < 0) accumA = 0;
	// add the accumulator to pixArray_overlay at index i and j
	p->pixArray_overlay[i][j].r = accumR;
	p->pixArray_overlay[i][j].g = accumG;
	p->pixArray_overlay[i][j].b = accumB;
	p->pixArray_overlay[i][j].a = accumA;
}

static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	//reverse the pixels vertically - can be done in one line
	memcpy(p->pixArray_overlay[i] + j, oldPixMap->pixArray_overlay[oldPixMap->imageHeight - i - 1] + j, sizeof(rgba));
}

static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	//reverse the pixels horizontally - can be done in one line
	memcpy(p->pixArray_overlay[i] + j, oldPixMap->pixArray_overlay[i] + oldPixMap->imageWidth - j - 1, sizeof(rgba));
}
