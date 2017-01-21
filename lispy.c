#include "mpc.h"
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>

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
/*#include <editline/history.h>*/
#endif
/*#include <editline/history.h>*/

enum {LERR_DIV_ZERO, LERR_BAD_NUM, LERR_BAD_OP};
enum {LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR};

typedef struct lval{
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct lval** cell;
} lval;

/* Function declaration*/
void lval_print(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i);
lval* builtin_op(lval* a, char* op);
lval* lval_take(lval* v, int i);
lval* lval_eval(lval* v);


lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(sizeof(strlen(m) + 1));
    strcpy(v->err, m);
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(sizeof(strlen(s) + 1));
    strcpy(v->sym, s);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {

    switch (v->type) {
        /* Do nothing special for number type */
        case LVAL_NUM: break;

        /* for err or sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* if Sexpr then delte all elments inside */
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }

            free(v->cell);
        break;
    }

    /* Free the memory allocated for the lval struct itself */
    free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
        lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* If root (>) or secpr then create empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strcmp(t->tag, "sexpr"))  { x = lval_sexpr(); }

    /* FIll this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") ==0) { continue; }
        if (strcmp(t->children[i]->contents, ")") ==0) { continue; }
        if (strcmp(t->children[i]->contents, "{") ==0) { continue; }
        if (strcmp(t->children[i]->contents, "}") ==0) { continue; }
        if (strcmp(t->children[i]->tag, "regex") ==0)  { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval) * v->count);
    v->cell[v->count -1] = x;
    return v;
}

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        /* Print Value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if laste element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }

    putchar(close);
}

void lval_print(lval* v) {
    switch (v->type) {

    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_eval_sexpr(lval* v){ 

    /* Evalueate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty expression catch */
    if (v->count == 0) { return v; }

    /* single expression catch */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first Element is a Symbol */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression does not start with symbol!");
    }

    /* Call Builtin with operator */
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}


// lval eval_op(lval x, char* op, lval y) {

//     /* break if error passed in to x or y */
//     if (x.type == LVAL_ERR) { return x; }
//     if (y.type == LVAL_ERR) { return y; }


//     if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//     if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//     if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//     if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
//     if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
//     if (strcmp(op, "/") == 0) { 
//         return y.num == 0
//         ? lval_err(LERR_DIV_ZERO)
//         : lval_num(x.num / y.num);
//         }

//     return lval_err(LERR_BAD_OP);
// }


lval* lval_eval(lval* v) {
    /* Evaluate sexpressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
    /* ALl other eval types remain the same */
    return v;

    /* if tagged as number return directly */
    // if(strstr(t->tag, "number")) {

    //     errno = 0;
    //     long x = strtol(t->contents, NULL, 10);
    //     return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    // }

    // char* op = t->children[1]->contents;

    // lval x = eval(t->children[2]);

    // int i = 3;
    // while (strstr(t->children[i]->tag, "expr")) {
    //     x = eval_op(x, op, eval(t->children[i]));
    //     i++;
    // }

    // return x;
}

lval* lval_pop(lval* v, int i) {
    /* Find item at "i" */
    lval* x = v->cell[i];

    /*shift memory after the item at i over the top */
    memmove(&v->cell[i], &v->cell[i+1],
            sizeof(lval*) * (v->count-i-1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* builtin_op(lval* a, char* op) {

    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on a non number lmao!");
        }
    }

    /*Pop the first element */
    lval* x = lval_pop(a, 0);

    /* if no argyments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* do while elements still remain */
    while (a->count > 0) {

        /* pop the next element */
        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { return lval_num(x->num + y->num); }
        if (strcmp(op, "-") == 0) { return lval_num(x->num - y->num); }
        if (strcmp(op, "*") == 0) { return lval_num(x->num * y->num); }
        if (strcmp(op, "%") == 0) { return lval_num(x->num % y->num); }
        if (strcmp(op, "^") == 0) { return lval_num(pow(x->num, y->num)); }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division by zero!"); 
                break;
            } else {
                x->num /= y->num;
            }
        }

        lval_del(y);
    }

    lval_del(a);
    return x;

    
}

int main(int argc, char** argv) {

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol  = mpc_new("symbol");
    mpc_parser_t* Sexpression = mpc_new("sexpr");
    mpc_parser_t* Expression = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                            \
        number      : /-?[0-9]+/;                                \
        symbol      : '+' | '-' | '*' | '/' | '%' | '^';         \
        sexpr       : '(' <expr>* ')';                            \
        expr        : <number> | <symbol> | <sexpr>;            \
        lispy       : /^/ <expr>* /$/;                          \
    ",
    Number, Symbol, Sexpression, Expression, Lispy);

    /* do parsing here */
    puts("Lispy version 0.0.0.0.5");
    puts("Press  Ctrl+c to Exit\n");

    while (1) {

        char* input = readline("lispy> ");
        
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* on success print AST */
            // lval result = eval(r.output);
            // lval_println(result);
            // mpc_ast_delete(r.output);
            // lval* x = lval_read(r.output);
            // lval_println(x);
            // lval_del(x);
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            /*otherwise print error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /*free(input);*/
    }

    mpc_cleanup(5, Number, Symbol, Sexpression, Expression, Lispy);



    return 0;
}


