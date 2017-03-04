#include <stdio.h>
#include <string.h>
#include "pixutils.h"

#define MAXPLUGINS 64

int main(int argc, char *argv[]){
    char *inputfile=0,*outputfile=0;
    int bmpFlag=0;
    int i=1;
    int nPlugins=0;

    plugin *plugs[MAXPLUGINS];
    memset(plugs,0,sizeof(void*)*MAXPLUGINS);

    while (i<argc){
        if(! strcmp(argv[i],"-i")){
            inputfile=argv[i+1];
            i+=2;
        }
        else if(!strcmp(argv[i],"-o")){
            outputfile=argv[i+1];
            i+=2;
        }
        else if (!strcmp(argv[i],"-b")){
            bmpFlag=1;
            i++;
        }
        else if (argv[i][0] == '-' && argv[i][1] == '-'){
            if(!(plugs[nPlugins]=plugin_parse(argv,&i))){
                fprintf(stderr,"unable to parse %s\n", argv[i]);
                return 1;
            }
            nPlugins++;
        }
        else{
            fprintf(stderr,"unrecognized parmater %s\n",argv[i]);
            fprintf(stderr,"usage: transform -i inputfile -o outputfile --<filterfunctions> <optional parameters> \n");
            return 1;
        }
    }
    pixMap *p=pixMap_read(inputfile);
    if(!p){
        fprintf(stderr,"unable to allocate memory for pixMap\n");
        return 1;
    }
    for (int i=0;i<nPlugins;i++){
        pixMap_apply_plugin(p,plugs[i]);
    }
    if(bmpFlag) pixMap_write_bmp16(p,outputfile);
    else pixMap_write(p,outputfile);

    pixMap_destroy(&p);

    for (int i=0;i<nPlugins;i++){
        plugin_destroy(&(plugs[i]));
    }
    return 0;
}
