#include "bmp.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

BMP16map* BMP16map_init(int height,int width,int Abits,int Rbits,int Gbits, int Bbits){
 BMP16map *bmp16=malloc(sizeof(BMP16map));
 bmp16->pixArray=malloc(height*sizeof(uint16_t*));
 bmp16->width=width;
 bmp16->height=height;
 bmp16->rowPadding= (width*sizeof(uint16_t) %4)? 4-(width*sizeof(uint16_t)%4) : 0;
 bmp16->pixArray[0]=malloc(width*height*sizeof(uint16_t));
 memset(bmp16->pixArray[0],0, width*height*sizeof(uint16_t));
 for(int i=1;i<height;i++)
  bmp16->pixArray[i]=bmp16->pixArray[i-1]+width;
 bmp16->Abits=Abits;
 bmp16->Rbits=Rbits;
 bmp16->Gbits=Gbits;
 bmp16->Bbits=Bbits;
 return(bmp16);
}	
void BMP16map_destroy( BMP16map **bmp16){
	if(*bmp16 && (*bmp16)->pixArray && (*bmp16)->pixArray[0]){
		free((*bmp16)->pixArray[0]);
	}		
	if((*bmp16) && (*bmp16)->pixArray){
		free((*bmp16)->pixArray);
	}
	if(*bmp16)free(*bmp16);	
	*bmp16=0;
}	

	
void BMP16map_write(BMP16map *bmp16,char *filename){
	//reserve 3 or 4 bytes for bitfields in addition to 54
	int size=(bmp16->Abits)? 54+4*4 : 54+3*4;
	unsigned char *header=malloc(size);
	if(!header)exit(EXIT_FAILURE);
	memset(header,0,size);
	//write the necessary bitfields
 header[0]='B';header[1]='M';
 //sizes go here but in little endian byte order regardless of machine type
 uint32_t totalSize=size+(bmp16->width*sizeof(uint16_t)+bmp16->rowPadding)*bmp16->height;
 header[2]=totalSize & 0xFF;
 header[3]=(totalSize >> 8) & 0xFF;
 header[4]=(totalSize >> 16) & 0xFF;
 header[5]=(totalSize >> 24) & 0xFF;
 
 header[10]=size%256;header[11]=size/256;
 header[14]=40;
 
 header[18]=bmp16->width%256; header[19]=bmp16->width/256;
	header[22]=bmp16->height%256; header[23]=bmp16->height/256;
 header[26]=1;header[28]=16;header[30]=3;
 header[34]=(totalSize-size) & 0xFF;
 header[35]=((totalSize-size) >> 8) & 0xFF;
 header[36]=((totalSize-size) >> 16) & 0xFF;
 header[37]=((totalSize-size) >> 24) & 0xFF;
 //bit fields are 32 bit integers - little endian - do bit math on 32 bits
 uint32_t bf=0;
 if(bmp16->Abits){ 
		bf=((1<<bmp16->Bbits)-1);
  memcpy(header+54+8,&bf,4); 
  bf=((1<<(bmp16->Gbits))-1) << (bmp16->Bbits);
  memcpy(header+54+4,&bf,4);
  bf=((1<<(bmp16->Rbits))-1) << (bmp16->Bbits+bmp16->Gbits);
  memcpy(header+54,&bf,4);
  bf=((1<<(bmp16->Abits))-1) << (bmp16->Bbits+bmp16->Gbits+bmp16->Rbits);
  memcpy(header+54+12,&bf,4);
	}
	else{
  bf=((1<<bmp16->Bbits)-1);
  memcpy(header+54+8,&bf,4); 
  bf=((1<<(bmp16->Gbits))-1) << (bmp16->Bbits);
  memcpy(header+54+4,&bf,4);
  bf=((1<<(bmp16->Rbits))-1) << (bmp16->Bbits+bmp16->Gbits);
  memcpy(header+54,&bf,4);
	}		
 
 FILE *fp=fopen(filename,"w");
 fwrite(header,1,size,fp);
 unsigned char a[4]={0,0,0,0};
 for(int i=0;i<bmp16->height;i++){
  fwrite(bmp16->pixArray[i],sizeof(uint16_t),bmp16->width,fp);
  fwrite(a,bmp16->rowPadding,1,fp);
	}
	fclose(fp);
	free(header);
}

