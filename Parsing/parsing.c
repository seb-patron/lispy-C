#include "mpc.h"
#include <stdio.h> 
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char input[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin;
    char* cpy = malloc(strlen(buffer) +1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\o';
    return cpy;
}

void add_history(char* unused) {}
/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
/*#include <editline/history.h> */
#endif
/*#include <editline/history.h>*/

int main(int argc, char** argv) {

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                            \
        number      : /-?[0-9]+/;                                \
        operator    : '+' | '-' | '*' | '/' | '%';               \
        expression  : <number> | '('<operator> <expression>+ ')';\
        lispy       : /^/ <operator> <expression>+ /$/;          \
    ",
    Number, Operator, Expression, Lispy);

    /* do parsing here */
    puts("Lispy version 0.0.0.0.1");
    puts("Press  Ctrl+c to Exit\n");

    while (1) {

        char* input = readline("lispy> ");
        
        add_history(input);

        /*printf("no you're a %s\n", input);*/
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* on success print AST */
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            /*otherwise print error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expression, Lispy);



    return 0;
}


long eval(mpc_ast_t* t) {

    /* if tagged as number return directly */
    if(strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    char* op = t->children[1]->contents;

    long x = eval(t->children[2]);

    int i = 3;
    while (strstr(i->children[i]->tag, "expression")) {
        x = eval_op(x, op, eval(t->contents[i]));
    }

    return x;
}

long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    return 0;
}