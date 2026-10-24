// EMRECAN ULU 251ADB247
// https://github.com/programmerulu/Graded-lab-Arithmetic-Parser-in-C_251ABD247_EmrecanUlu
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define STUDENT_NAME  "emrecan"
#define STUDENT_LASTNAME  "ulu"
#define STUDENT_NUMBER  "251ADB247"



char *read_file(const char *path, size_t *out_len){
  FILE*f=fopen(path,"rb");
    if(f==NULL){
      return NULL;
    }


  size_t cap=16;
  size_t len=0;
  char *buf = malloc(cap + 1);
  if(buf==NULL){
    fclose(f);
    return NULL;
  }

  for(;;){
    size_t got= fread(buf+len,1,cap-len,f);
    len+=got;
    if(len<cap){
        break;
        
    }
    cap*=2;
    char *tmp = realloc(buf,cap +1);
        if (tmp == NULL) {
            free(buf);
            fclose(f);
            return NULL;
}
    buf = tmp;
  }

  buf[len]='\0';
  fclose(f);
  *out_len=len;
  return buf;
  
}

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

  size_t len=0;
  char *text= read_file(input,&len);
  if(text==NULL){
    fprintf(stderr,"Cannot read '%s'\n",input);
    return 1;
  }
  
  printf("%zu bytes read\n", len);

  const char *slash = strrchr(input, '/');
  const char*filename;

  if(slash==NULL){
    filename=input;
  }

  else{
    filename=slash+1;
  }

  printf("%s\n", filename);



  free(text);
  return 0;
} 