// EMRECAN ULU 251ADB247
// https://github.com/programmerulu/Graded-lab-Arithmetic-Parser-in-C_251ABD247_EmrecanUlu
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

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

typedef enum { TOK_NUM, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_EOL, TOK_BAD } TokType;

typedef struct {
    TokType type;
    size_t pos;
    long long value;
} Token;

Token next_token(const char *text, size_t len, size_t *p)
{
    Token t;
    t.value = 0;

    while (*p < len && (text[*p] == ' ' || text[*p] == '\t')) (*p)++;
    t.pos = *p;

    if (*p >= len || text[*p] == '\n') {
        t.type = TOK_EOL;
        return t;
    }

    char c = text[*p];
    if (c >= '0' && c <= '9') {
        t.type = TOK_NUM;
        while (*p < len && text[*p] >= '0' && text[*p] <= '9') {
            t.value = t.value * 10 + (text[*p] - '0');
            (*p)++;
        }
        return t;
    }
    if (c == '+') { t.type = TOK_PLUS;  (*p)++; return t; }
    if (c == '-') { t.type = TOK_MINUS; (*p)++; return t; }
    if (c == '*') { t.type = TOK_STAR;  (*p)++; return t; }
    if (c == '/') { t.type = TOK_SLASH; (*p)++; return t; }


    t.type = TOK_BAD;
    (*p)++;
    return t;
}

size_t parse_term(const char *text, size_t len, size_t *p, long long *out)
{
    Token t = next_token(text, len, p);
    if (t.type != TOK_NUM) return t.pos + 1;
    long long value = t.value;

    
    for (;;) {
        size_t q = *p;
        Token op = next_token(text, len, &q);
        if (op.type != TOK_STAR && op.type != TOK_SLASH) break;
        *p = q; 

        t = next_token(text, len, p);
        if (t.type != TOK_NUM) return t.pos + 1;

        if (op.type == TOK_STAR) {
            value *= t.value;
        } else {
           if (t.value == 0) return t.pos + 1;
            value /= t.value;
        }
    }
    *out = value;
    return 0;
}


size_t eval_line(const char *text, size_t len, size_t start, long long *result)
{
    size_t p = start;
    long long value = 0;
    size_t err = parse_term(text, len, &p, &value);
    if (err != 0) return err;

    for (;;) {
        Token t = next_token(text, len, &p);
        if (t.type == TOK_EOL) break;
        if (t.type != TOK_PLUS && t.type != TOK_MINUS) return t.pos + 1;
        TokType op = t.type;

        long long rhs = 0;
        err = parse_term(text, len, &p, &rhs);
        if (err != 0) return err;
        if (op == TOK_PLUS) value += rhs;
        else                value -= rhs;
    }
    *result = value;
    return 0;
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
  

  const char *slash = strrchr(input, '/');
  const char*filename;

  if(slash==NULL){
    filename=input;
  }

  else{
    filename=slash+1;
  }


  size_t n = strlen(filename);
  size_t baselen = n;
  if (n > 4 && strcmp(filename + n - 4, ".txt") == 0) {
    baselen = n - 4;
     }

  char defdir[512];
  if(outdir==NULL){
    snprintf(defdir, sizeof defdir, "%.*s_%s_%s", (int)baselen,filename,STUDENT_NAME,STUDENT_NUMBER);
    outdir=defdir;
  }

  mkdir(outdir,0777);

  char outpath[1024];
  snprintf(outpath, sizeof outpath, "%s/%.*s_%s_%s_%s.txt",outdir, (int)baselen, filename, STUDENT_NAME, STUDENT_LASTNAME, STUDENT_NUMBER);
  FILE *out = fopen(outpath, "wb");
  if (out == NULL) {
    fprintf(stderr, "Cannot write '%s'\n", outpath);
    free(text);
    return 1;
}
  

  size_t i = 0;
 while (i < len) {
    size_t s = i;
    while (s < len && (text[s] == ' ' || text[s] == '\t')) s++;
    if (s >= len) break;
    if (text[s] == '\n') {
        i = s + 1;
        continue;
    }

    size_t e = s;
    while (e < len && text[e] != '\n') e++;

    long long result = 0;
    size_t err = eval_line(text, len, s, &result);
    if (err != 0) {
        fclose(out);
        out = fopen(outpath, "wb");      // dosyayı sıfırla: önceki sonuçlar silinsin
        if (out == NULL) { free(text); return 1; }
        fprintf(out, "ERROR:%zu\n", err);
        break;                           // ilk hatada dur
    }
    fprintf(out, "%lld\n", result);

    i = e + 1;
}

  fclose(out);




  free(text);
  return 0;
} 