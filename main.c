#include <stdio.h>
#include <stdarg.h>

extern FILE *yyin;
extern int yylineno;

extern int yyparse();

extern FILE *ast_out;
FILE *out;

int main(int argc, char *argv[]) {
    if (argc > 1) {
        FILE *file;
        file = fopen(argv[1], "r");
        if (file) {
            yyin = file;
        }
    }
    ast_out = stdout;
    if (argc > 2) {
        FILE *file;
        file = fopen(argv[2], "wb");
        if (file) {
            ast_out = file;
        }
    }

    yylineno = 1;

    yyparse();
    return 0;
}

void yyerror(char *str, ...) {
    va_list ap;
    va_start(ap, str);

    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, str, ap);
    fprintf(stderr, "\n");
}