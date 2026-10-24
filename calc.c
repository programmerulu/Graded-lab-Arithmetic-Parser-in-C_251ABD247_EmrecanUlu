// EMRECAN ULU 251ADB247
// https://github.com/programmerulu/Graded-lab-Arithmetic-Parser-in-C_251ABD247_EmrecanUlu
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>


#include <stdio.h>
#include <string.h>



int main(int argc, char**argv) {
   const char *outdir=NULL;
   const char *input=NULL;
  
    for(int i=1; i < argc; i++){

    if(strcmp(argv[i],"-o") == 0){

        if(argv[i+1]!=NULL){
            outdir=argv[i+1];
            i++;
        }
        else{
            fprintf(stderr,"Missing argument.\n");
            return 1;
        }
    }

    else if(argv[i][0]=='-'){

        fprintf(stderr,"Unknown option.\n");
        return 1;
    }

    else{

        input=argv[i];
    }

    
  }
  

  if(input==NULL){

    fprintf(stderr,"There is no input by user.\n");
    return 1;
  }

  FILE*f=fopen(input,"rb");
  if(f==NULL){
    fprintf(stderr,"Cannot open '%s'\n",input);
    return 1;
  }

  fclose(f);
  return 0;
} 