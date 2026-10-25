// EMRECAN ULU 251ADB247
// https://github.com/programmerulu/Graded-lab-Arithmetic-Parser-in-C_251ADB247_EmrecanUlu
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic AI, conversation: https://claude.ai/share/f5481af9-6b60-461e-9bd0-90fd70238674),
//                https://www.w3schools.com/c/index.php,
//                https://www.geeksforgeeks.org/c/c-programming-language/


#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>

#define STUDENT_NAME  "emrecan"
#define STUDENT_LASTNAME  "ulu"
#define STUDENT_NUMBER  "251ADB247"


// It reads the whole file into ONE buffer. 
// When the buffer is full, its size doubles (becomes two times bigger). 
// This way, copying is fast. 
// If it grows by the same amount every time, it will copy the data again and again. 
// This is very slow for big files.
// We keep one extra byte at the end for the '\0' character.

// AI (Claude): prompt "mallocun ne oldugunu bilmiyorum ... free napiyor buffu mu bosaltiyor"
// - explained malloc/realloc/free and the doubling idea; I wrote the loop myself.

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



typedef enum { TOK_NUM, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_POW, TOK_LPAREN, TOK_RPAREN, TOK_EOL, TOK_BAD } TokType;

// A number that remembers whether it is an integer or a float.
typedef struct { int is_float; long long i; double d; } Value;

// 'pos' is the position of the token's first character in the WHOLE file, not in the line.
// New lines and comments are also in the file buffer.
// So, "pos + 1" is exactly the position (starting from 1) that the lab wants.
// This way, we do not need line or column counters that can make mistakes.

typedef struct { TokType type; size_t pos; Value num; } Token;


// It reads one token starting at text[*p]. Then, it moves *p after the token.
// End of line is a real token (TOK_EOL). It stays ON the '\n' character.
// (If the last line has no '\n', it stays at the end of the file).
// This gives the correct position for "missing operand" and "missing )" errors.
// We don't need special code for this: the parser just gives an error for the EOL token.

// AI (Claude): the Value type, the tokenizer, the recursive descent functions and
// process_dir were written by Claude in answer to my requests for each grade step
// (e.g. "Not 7 parantezler ... bunu nasil yapacagim"); I integrated, tested and debugged them.

Token next_token(const char *text, size_t len, size_t *p)
{

    Token t;
    t.num.is_float = 0; t.num.i = 0; t.num.d = 0.0;
    while (*p < len && (text[*p] == ' ' || text[*p] == '\t')) (*p)++;
    t.pos = *p;
    if (*p >= len || text[*p] == '\n') { t.type = TOK_EOL; return t; }
    char c = text[*p];
    if (c >= '0' && c <= '9') {
        size_t start = *p;
        unsigned long long acc = 0;       // unsigned: wrapping is defined, no UB
        t.type = TOK_NUM;
        while (*p < len && text[*p] >= '0' && text[*p] <= '9') {
            acc = acc * 10 + (unsigned long long)(text[*p] - '0');
            (*p)++;
        }
        t.num.i = (long long)acc;
        // A float needs digits on BOTH sides of the dot. If '.' is not followed by a
        // digit we leave it alone: the next token then starts at the '.', which is not
        // a valid token, so the error lands exactly on the '.' as the lab requires.
        if (*p + 1 < len && text[*p] == '.' && text[*p + 1] >= '0' && text[*p + 1] <= '9') {
            (*p)++;
            while (*p < len && text[*p] >= '0' && text[*p] <= '9') (*p)++;
            // strtod needs a NUL-terminated string, so copy just this token.
            size_t n = *p - start;
            char small[64];
            char *tmp = (n < sizeof small) ? small : malloc(n + 1);
            if (tmp == NULL) { t.type = TOK_BAD; return t; }
            memcpy(tmp, text + start, n);
            tmp[n] = '\0';
            double d = strtod(tmp, NULL);
            if (tmp != small) free(tmp);
            if (!isfinite(d)) { t.type = TOK_BAD; return t; }
            t.num.is_float = 1;
            t.num.d = d;
        }
        return t;
    }
    if (c == '+') { t.type = TOK_PLUS;  (*p)++; return t; }
    if (c == '-') { t.type = TOK_MINUS; (*p)++; return t; }
    if (c == '*') {

        // Longest match: "**" is one token, so "2 * * 3" (with a space) stays two STAR
        // tokens and fails as two operators in a row, as the lab requires.
        if (*p + 1 < len && text[*p + 1] == '*') { t.type = TOK_POW; (*p) += 2; return t; }
        t.type = TOK_STAR; (*p)++; return t;
    }
    if (c == '/') { t.type = TOK_SLASH; (*p)++; return t; }
    if (c == '(') { t.type = TOK_LPAREN; (*p)++; return t; }
    if (c == ')') { t.type = TOK_RPAREN; (*p)++; return t; }
    t.type = TOK_BAD; (*p)++; return t;
}

static double as_double(Value v) { return v.is_float ? v.d : (double)v.i; }

// + - * / on two Values. Returns 0 on success or the 1-based error position.
// op_pos: the operator (float overflow/NaN is reported there).
// rhs_pos: start of the right operand (division by zero is reported there).
// Integer or float: if both operands are integers we stay in long long, so 7 / 2 is 3
// (C truncates toward zero). As soon as one operand is a float, both become double,
// so 7 / 2.0 is 3.5.
// Unsigned arithmetic is used for integers because it wraps around on overflow instead of
// being undefined behaviour in C, so the sanitizer build stays quiet.

size_t apply_arith(TokType op, size_t op_pos, size_t rhs_pos, Value a, Value b, Value *out)
{
    Value r = {0, 0, 0.0};
    if (!a.is_float && !b.is_float) {              // both integers: stay in long long
        unsigned long long ua = (unsigned long long)a.i, ub = (unsigned long long)b.i;
        if (op == TOK_PLUS)       r.i = (long long)(ua + ub);
        else if (op == TOK_MINUS) r.i = (long long)(ua - ub);
        else if (op == TOK_STAR)  r.i = (long long)(ua * ub);
        else {
            if (b.i == 0) return rhs_pos + 1;
            if (b.i == -1) r.i = (long long)(0ULL - ua);   // avoids LLONG_MIN / -1 overflow
            else r.i = a.i / b.i;
        }
        *out = r;
        return 0;
    }
    double x = as_double(a), y = as_double(b), z = 0.0;   // any float: switch to double
    if (op == TOK_PLUS)       z = x + y;
    else if (op == TOK_MINUS) z = x - y;
    else if (op == TOK_STAR)  z = x * y;
    else {
        if (y == 0.0) return rhs_pos + 1;
        z = x / y;
    }
    if (!isfinite(z)) return op_pos + 1;
    r.is_float = 1; r.d = z;
    *out = r;
    return 0;
}

// We use a fast method to calculate integer powers.
// Because of this, even a huge calculation like 2 ** 1000000000 is very fast.
// Any number to the power of 0 (like x ** 0 or 0 ** 0) is exactly 1.
// If you use a negative integer power, the program gives an error.
// This error points to the beginning of the power number.
// For decimal numbers (floats), we use the standard pow() function.
// If the answer is too big (infinity) or not a valid number (NaN), it gives an error.
// This error points to the '**' symbol.

size_t apply_pow(size_t op_pos, size_t exp_pos, Value base, Value ex, Value *out)
{
    Value r = {0, 0, 0.0};
    if (!base.is_float && !ex.is_float) {
        if (ex.i < 0) return exp_pos + 1;
        unsigned long long result = 1, b = (unsigned long long)base.i;
        long long e = ex.i;
        while (e > 0) { if (e & 1) result *= b; b *= b; e >>= 1; }
        r.i = (long long)result;
        *out = r;
        return 0;
    }
    double z = pow(as_double(base), as_double(ex));
    if (!isfinite(z)) return op_pos + 1;
    r.is_float = 1; r.d = z;
    *out = r;
    return 0;
}


// Grammar rules. We have one function for each math level (lowest priority first):
//   expr    := term  { ('+' | '-') term }
//   term    := power { ('*' | '/') power }
//   power   := primary [ '**' power ]
//   primary := NUMBER | '(' expr ')'
// 
// Each function only does its own math (like + or *). It gets numbers from the function below it.
// This way, more important operations happen first.
// For example, in "3 + 2 * 5", the '*' belongs to term(). It finishes "2 * 5" first.
// Then, expr() adds the result to 3.
// 
// +, -, *, / work from left to right because expr() and term() use a loop.
// ** (power) works from right to left because power() calls ITSELF.
// 
// We look at the next token using a copy (called 'q').
// If the token is not for us, we do not change '*p'. So, the calling function can still see it.

size_t parse_expr(const char *text, size_t len, size_t *p, Value *out);

// When the parser cannot continue, it gives a syntax error.
// It shows the error on the exact token it is looking at.
// For example, if a number is missing ("7 + * 3", "3 *") or a ')' is missing,
// the error points to that token.

size_t parse_primary(const char *text, size_t len, size_t *p, Value *out)
{
    Token t = next_token(text, len, p);
    if (t.type == TOK_NUM) { *out = t.num; return 0; }
    if (t.type == TOK_LPAREN) {
        size_t err = parse_expr(text, len, p, out);
        if (err != 0) return err;
        t = next_token(text, len, p);
        if (t.type != TOK_RPAREN) return t.pos + 1;
        return 0;
    }
    return t.pos + 1;
}

// power() calls itself for the exponent, so 2 ** 3 ** 2 groups as 2 ** (3 ** 2).
size_t parse_power(const char *text, size_t len, size_t *p, Value *out)
{
    Value base = {0, 0, 0.0};
    size_t err = parse_primary(text, len, p, &base);
    if (err != 0) return err;
    size_t q = *p;
    Token op = next_token(text, len, &q);
    if (op.type != TOK_POW) { *out = base; return 0; }
    *p = q;
    size_t q2 = *p;
    Token first = next_token(text, len, &q2);
    Value ex = {0, 0, 0.0};
    err = parse_power(text, len, p, &ex);
    if (err != 0) return err;
    return apply_pow(op.pos, first.pos, base, ex, out);
}

size_t parse_term(const char *text, size_t len, size_t *p, Value *out)
{
    Value value = {0, 0, 0.0};
    size_t err = parse_power(text, len, p, &value);
    if (err != 0) return err;
    for (;;) {
        size_t q = *p;
        Token op = next_token(text, len, &q);
        if (op.type != TOK_STAR && op.type != TOK_SLASH) break;
        *p = q;
        size_t q2 = *p;
        // We look at the first token on the right side, but we do not move past it.
        // If you try to divide by zero, the program gives an error.
        // This error points to the start of the right side (which is this token).
        Token first = next_token(text, len, &q2);
        Value rhs = {0, 0, 0.0};
        err = parse_power(text, len, p, &rhs);
        if (err != 0) return err;
        err = apply_arith(op.type, op.pos, first.pos, value, rhs, &value);
        if (err != 0) return err;
    }
    *out = value;
    return 0;
}

size_t parse_expr(const char *text, size_t len, size_t *p, Value *out)
{
    Value value = {0, 0, 0.0};
    size_t err = parse_term(text, len, p, &value);
    if (err != 0) return err;
    for (;;) {
        size_t q = *p;
        Token op = next_token(text, len, &q);
        if (op.type != TOK_PLUS && op.type != TOK_MINUS) break;
        *p = q;
        size_t q2 = *p;
        Token first = next_token(text, len, &q2);
        Value rhs = {0, 0, 0.0};
        err = parse_term(text, len, p, &rhs);
        if (err != 0) return err;
        err = apply_arith(op.type, op.pos, first.pos, value, rhs, &value);
        if (err != 0) return err;
    }
    *out = value;
    return 0;
}

// When a math expression is finished, there must be an end of line.
// If there is anything else (like an extra ')' or a second number), the parser stops.
// It gives an error on this extra token because it cannot continue.


size_t eval_line(const char *text, size_t len, size_t start, Value *result)
{
    size_t p = start;
    Value value = {0, 0, 0.0};
    size_t err = parse_expr(text, len, &p, &value);
    if (err != 0) return err;
    Token t = next_token(text, len, &p);
    if (t.type != TOK_EOL) return t.pos + 1;
    *result = value;
    return 0;
}



void split_name(const char *path, const char **filename, size_t *baselen)
{
    const char *slash = strrchr(path, '/');
    *filename = (slash == NULL) ? path : slash + 1;
    size_t n = strlen(*filename);
    *baselen = n;
    if (n > 4 && strcmp(*filename + n - 4, ".txt") == 0) *baselen = n - 4;
}

// The input is read into memory once (see read_file), but results are written to the output
// file line by line, so they are never collected in memory.
// If there is an error, we open the file again with "wb". This deletes everything inside.
// Then, we only write "ERROR:<pos>". So, we do not leave any incomplete results.
// We do not write anything for empty lines.
// If a line starts with '#', we skip it. But we still count its characters for the position.


int process_file(const char *input, const char *outdir)
{
    size_t len = 0;
    char *text = read_file(input, &len);
    if (text == NULL) {
        fprintf(stderr, "Cannot read '%s'\n", input);
        return 1;
    }

    const char *filename;
    size_t baselen;
    split_name(input, &filename, &baselen);

    char outpath[1024];
    snprintf(outpath, sizeof outpath, "%s/%.*s_%s_%s_%s.txt", outdir, (int)baselen, filename,
             STUDENT_NAME, STUDENT_LASTNAME, STUDENT_NUMBER);
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

        if (text[s] == '#') {
            size_t k = s;
            while (k < len && text[k] != '\n') k++;
            i = k + 1;
            continue;
        }

        size_t e = s;
        while (e < len && text[e] != '\n') e++;

        Value result = {0, 0, 0.0};
        size_t err = eval_line(text, len, s, &result);
        if (err != 0) {
            fclose(out);
            out = fopen(outpath, "wb"); 
            if (out == NULL) { free(text); return 1; }
            fprintf(out, "ERROR:%zu\n", err);
            break;
        }
            if (result.is_float) fprintf(out, "%.15g\n", result.d);
        else   fprintf(out, "%lld\n", result.i);

        i = e + 1;
    }

    fclose(out);
    free(text);
    return 0;
}


// It only reads normal '.txt' files in the folder.
// If a folder has a '.txt' name, the program ignores it.
// We do not look inside other folders.
// If one file gives an error, the program does not stop. It checks the other files.
// But at the end, the program returns an error code (1).

int process_dir(const char *dir, const char *outdir)
{
    DIR *d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "Cannot open folder '%s'\n", dir);
        return 1;
    }

    int rc = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        size_t n = strlen(name);
        if (n <= 4 || strcmp(name + n - 4, ".txt") != 0) continue;

        char path[1024];
        snprintf(path, sizeof path, "%s/%s", dir, name);

        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue; 

        if (process_file(path, outdir) != 0) rc = 1;
    }
    closedir(d);
    return rc;
}

int main(int argc, char **argv)
{
    const char *outdir = NULL, *input = NULL, *dir = NULL;


    // AI (Claude): prompt "Komut satirindan nasil -o OUTDIR input.txt alirim" - explained
    // argc/argv; I wrote the option loop myself and fixed it with feedback.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Missing argument for '%s'\n", argv[i]);
                return 1;
            }
            outdir = argv[++i];
        }
                else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Missing argument for '%s'\n", argv[i]);
                return 1;
            }
            dir = argv[++i];
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option '%s'\n", argv[i]);
            return 1;
        }
        else {
            input = argv[i];
        }
    }

    if (input == NULL && dir == NULL) {
        fprintf(stderr, "usage: calc [-d DIR] [-o OUTDIR] [input.txt]\n");
        return 1;
    }
    if (input != NULL && dir != NULL) {
        fprintf(stderr, "Give either an input file or -d DIR, not both\n");
        return 1;
    }

    const char *src = (dir != NULL) ? dir : input;

   // We must check the input BEFORE we create the output folder.
   // If there is no input, the program will make an empty folder. We want to prevent this.

    struct stat st;
    if (stat(src, &st) != 0) {
        fprintf(stderr, "Cannot access '%s'\n", src);
        return 1;
    }
    if (dir != NULL && !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "'%s' is not a folder\n", dir);
        return 1;
    }
    if (input != NULL && !S_ISREG(st.st_mode)) {
        fprintf(stderr, "'%s' is not a file\n", input);
        return 1;
    }

    char defdir[1024];
    if (outdir == NULL) {
        char srccopy[1024];
        snprintf(srccopy, sizeof srccopy, "%s", src);
        size_t sl = strlen(srccopy);
        while (sl > 1 && srccopy[sl - 1] == '/') srccopy[--sl] = '\0';   // "labs/" -> "labs"

        const char *filename;
        size_t baselen;
        split_name(srccopy, &filename, &baselen);
        if (dir != NULL) baselen = strlen(filename);   // a folder name is used as-is; ".txt" is only stripped from file names

        snprintf(defdir, sizeof defdir, "%.*s_%s_%s", (int)baselen, filename, STUDENT_NAME, STUDENT_NUMBER);
        outdir = defdir;
    }

    if (mkdir(outdir, 0777) != 0 && errno != EEXIST) {   // an existing output folder is fine
        fprintf(stderr, "Cannot create folder '%s'\n", outdir);
        return 1;
    }

    if (dir != NULL) return process_dir(dir, outdir);
    return process_file(input, outdir);
}