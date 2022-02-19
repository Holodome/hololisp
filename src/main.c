#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    OBJ_NONE,
    OBJ_NUMI,
    OBJ_CONS,
    OBJ_SYMB,
    OBJ_FUNC,
    OBJ_NIL,
    OBJ_PRIMITIVE,
    OBJ_ENV,
    OBJ_MACRO,
    OBJ_TRUE
};

struct lisp_runtime;
struct lisp_obj;
typedef struct lisp_obj *primitive_func(struct lisp_runtime *rt,
                                        struct lisp_obj *list);

typedef struct lisp_obj {
    int kind;

    union {
        // Number
        long long numi;
        // Cons
        struct {
            struct lisp_obj *car;
            struct lisp_obj *cdr;
        };
        // Function definition
        struct {
            struct lisp_obj *params;
            struct lisp_obj *body;
            struct lisp_obj *env;
        };
        // Stack
        struct {
            struct lisp_obj *vars;
            struct lisp_obj *up;
        };
        // Symbol name
        char symb[1];
        // Primitive func
        primitive_func *prim;
    };
} lisp_obj;

typedef struct lisp_runtime {
    lexer *lex;

    lisp_obj *locals;
    lisp_obj *nil;
    lisp_obj *dot;
} lisp_runtime;

lisp_obj *
make_cons(lisp_obj *car, lisp_obj *cdr) {
    lisp_obj *cons = calloc(1, sizeof(lisp_obj));
    cons->kind     = OBJ_CONS;
    cons->car      = car;
    cons->cdr      = cdr;
    return cons;
}

lisp_obj *read_expr(lisp_runtime *rt);

lisp_obj *
intern(lisp_runtime *rt, char *symb) {
    lisp_obj *obj;
    for (obj = rt->locals; obj != rt->nil; obj = obj->cdr) {
        if (strcmp(obj->symb, symb) == 0) {
            break;
        }
    }

    if (obj == rt->nil) {
        int len   = strlen(symb);
        obj       = calloc(1, sizeof(lisp_obj) + len);
        obj->kind = OBJ_SYMB;
        memcpy(obj->symb, symb, len);
    }

    return obj;
}

lisp_obj *
reverse_list(lisp_runtime *rt, lisp_obj *obj) {
    lisp_obj *ret = rt->nil;
    while (obj != rt->nil) {
        lisp_obj *head = obj;
        obj            = obj->cdr;
        head->cdr      = ret;
        ret            = head;
    }
    return ret;
}

lisp_obj *
read_num(lisp_runtime *rt) {
    assert(rt->lex->kind == TOK_NUMI);
    lisp_obj *num = calloc(1, sizeof(lisp_obj));
    num->kind     = OBJ_NUMI;
    num->numi     = rt->lex->numi;
    lexer_eat(rt->lex);
    return num;
}

lisp_obj *
read_cons(lisp_runtime *rt) {
    lisp_obj *expr = rt->nil;

    lexer_peek(rt->lex);
    assert(rt->lex->kind = TOK_LPAREN);
    lexer_eat_peek(rt->lex);

    if (rt->lex->kind == TOK_RPAREN) {
        lexer_eat(rt->lex);
        goto out;
    }

    for (;;) {
        lexer_peek(rt->lex);
        if (rt->lex->kind == TOK_RPAREN) {
            lexer_eat_peek(rt->lex);
            expr = reverse_list(rt, expr);
            break;
        } else if (rt->lex->kind == TOK_DOT) {
            lexer_eat_peek(rt->lex);
            lisp_obj *car = read_expr(rt);
            expr          = reverse_list(rt, make_cons(car, expr));
            lexer_peek(rt->lex);
            assert(rt->lex->kind == TOK_RPAREN);
            lexer_eat(rt->lex);
            break;
        }

        lisp_obj *car = read_expr(rt);
        expr          = make_cons(car, expr);
    }

out:
    return expr;
}

lisp_obj *
read_quote(lisp_runtime *rt) {
    lisp_obj *sym = intern(rt, "quote");
    lexer_eat(rt->lex);
    return make_cons(sym, make_cons(read_expr(rt), rt->nil));
}

lisp_obj *
read_expr(lisp_runtime *rt) {
    lisp_obj *expr = rt->nil;

    lexer_peek(rt->lex);
    switch (rt->lex->kind) {
    default:
        assert(false);
    case TOK_EOF:
        break;
    case TOK_QUOTE:
        expr = read_quote(rt);
        break;
    case TOK_NUMI:
        expr = read_num(rt);
        break;
    case TOK_LPAREN:
        expr = read_cons(rt);
        break;
    case TOK_SYMB:
        expr = intern(rt, rt->lex->tok_buf);
        lexer_eat(rt->lex);
        break;
    }

    return expr;
}

void
print(lisp_runtime *rt, lisp_obj *obj) {
    switch (obj->kind) {
    default:
        assert(false);
    case OBJ_CONS:
        printf("(");
        for (;;) {
            print(rt, obj->car);
            if (obj->cdr == rt->nil) {
                break;
            }

            if (obj->cdr->kind != OBJ_CONS) {
                printf(" . ");
                print(rt, obj->cdr);
                break;
            }

            printf(" ");
            obj = obj->cdr;
        }
        printf(")");
        break;
    case OBJ_NUMI:
        printf("%lld", obj->numi);
        break;
    case OBJ_SYMB:
        printf("%s", obj->symb);
        break;
    case OBJ_NIL:
        printf("()");
        break;
    }
}

int
main(void) {
    char *filename = "examples/stdlib.lisp";

    FILE *f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *file_data = calloc(size + 1, 1);
    fread(file_data, size, 1, f);
    fclose(f);

    char tok_buf[4096];
    lexer lex = make_lexer(file_data, tok_buf, sizeof(tok_buf));

    lisp_runtime rt = {.lex = &lex, .nil = &(lisp_obj){.kind = OBJ_NIL}};
    rt.locals       = &(lisp_obj){.kind = OBJ_ENV};

    while (lexer_peek(&lex)) {
        lisp_obj *cons = read_expr(&rt);
        print(&rt, cons);
        printf("\n");
        fflush(stdout);
    }

    return 0;
}
