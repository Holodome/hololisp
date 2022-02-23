#include "lisp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static lisp_obj *read_expr(lisp_runtime *runtime);

static lisp_obj *
find_symb(lisp_runtime *runtime, char *symb) {
    lisp_obj *obj;
    for (obj = runtime->obarray; obj != runtime->nil; obj = obj->cdr) {
        if (strcmp(obj->car->symb, symb) == 0) {
            break;
        }
    }

    if (obj == runtime->nil) {
        obj = lisp_make_symbol(runtime, symb);
        runtime->obarray = lisp_make_cons(runtime, obj, runtime->obarray);
    } else {
        obj = obj->car;
    }
    return obj;
}

lisp_obj *
reverse_list(lisp_runtime *runtime, lisp_obj *obj) {
    lisp_obj *ret = runtime->nil;
    while (obj != runtime->nil) {
        lisp_obj *head = obj;
        obj = obj->cdr;
        head->cdr = ret;
        ret = head;
    }
    return ret;
}

lisp_obj *
read_cons(lisp_runtime *runtime) {
    lisp_obj *expr = runtime->nil;

    lisp_lexer_peek(runtime->lexer);
    assert(runtime->lexer->kind = TOK_LPAREN);
    lisp_lexer_eat_peek(runtime->lexer);

    if (runtime->lexer->kind == TOK_RPAREN) {
        lisp_lexer_eat(runtime->lexer);
        goto out;
    }

    for (;;) {
        lisp_lexer_peek(runtime->lexer);
        if (runtime->lexer->kind == TOK_RPAREN) {
            lisp_lexer_eat_peek(runtime->lexer);
            expr = reverse_list(runtime, expr);
            break;
        } else if (runtime->lexer->kind == TOK_DOT) {
            lisp_lexer_eat_peek(runtime->lexer);
            lisp_obj *car = read_expr(runtime);
            expr = reverse_list(runtime, lisp_make_cons(runtime, car, expr));
            lisp_lexer_peek(runtime->lexer);
            assert(runtime->lexer->kind == TOK_RPAREN);
            lisp_lexer_eat(runtime->lexer);
            break;
        }

        lisp_obj *car = read_expr(runtime);
        expr = lisp_make_cons(runtime, car, expr);
    }

out:
    return expr;
}

lisp_obj *
read_quote(lisp_runtime *runtime) {
    lisp_obj *sym = find_symb(runtime, "quote");
    lisp_lexer_eat(runtime->lexer);
    return lisp_make_cons(
        runtime, sym,
        lisp_make_cons(runtime, read_expr(runtime), runtime->nil));
}

lisp_obj *
read_expr(lisp_runtime *runtime) {
    lisp_obj *expr = runtime->nil;

    lisp_lexer_peek(runtime->lexer);
    switch (runtime->lexer->kind) {
    default:
        assert(false);
    case TOK_EOF:
        break;
    case TOK_QUOTE:
        expr = read_quote(runtime);
        break;
    case TOK_NUMI:
        expr = lisp_make_num(runtime, runtime->lexer->numi);
        lisp_lexer_eat(runtime->lexer);
        break;
    case TOK_LPAREN:
        expr = read_cons(runtime);
        break;
    case TOK_SYMB:
        expr = find_symb(runtime, runtime->lexer->tok_buf);
        lisp_lexer_eat(runtime->lexer);
        break;
    }

    return expr;
}

lisp_runtime *
lisp_create(void) {
    lisp_runtime *runtime = calloc(1, sizeof(lisp_runtime));

    lisp_obj *nil = calloc(1, sizeof(lisp_obj));
    nil->kind = LOBJ_NIL;
    runtime->nil = nil;

    lisp_obj *env = lisp_make_env(runtime, NULL);
    runtime->env = env;

    runtime->lexer = calloc(1, sizeof(lisp_lexer));
    runtime->obarray = runtime->nil;

    return runtime;
}

void
lisp_exec_buffer(lisp_runtime *runtime, char *buffer) {
    char tok_buf[4096];
    lisp_lexer_init(runtime->lexer, buffer, tok_buf, sizeof(tok_buf));

    while (lisp_lexer_peek(runtime->lexer)) {
        lisp_obj *cons = read_expr(runtime);
        lisp_print(runtime, cons);
        printf("\n");
        fflush(stdout);
    }
}

lisp_obj *
lisp_make_cons(lisp_runtime *runtime, lisp_obj *car, lisp_obj *cdr) {
    (void)runtime;
    lisp_obj *obj = calloc(1, sizeof(lisp_obj));
    obj->kind = LOBJ_CONS;
    obj->car = car;
    obj->cdr = cdr;
    return obj;
}

lisp_obj *
lisp_make_num(lisp_runtime *runtime, int64_t value) {
    (void)runtime;
    lisp_obj *obj = calloc(1, sizeof(lisp_obj));
    obj->kind = LOBJ_NUMI;
    obj->numi = value;
    return obj;
}

lisp_obj *
lisp_make_env(lisp_runtime *runtime, lisp_obj *up) {
    (void)runtime;
    lisp_obj *obj = calloc(1, sizeof(lisp_obj));
    obj->kind = LOBJ_ENV;
    obj->up = up;
    return obj;
}

lisp_obj *
lisp_make_symbol(lisp_runtime *runtime, char *symb) {
    (void)runtime;
    uint32_t len = strlen(symb);
    lisp_obj *obj = calloc(1, sizeof(lisp_obj) + len);
    obj->kind = LOBJ_SYMB;
    memcpy(obj->symb, symb, len);
    return obj;
}

lisp_obj *
lisp_make_func(lisp_runtime *runtime, lisp_obj *params, lisp_obj *body,
               lisp_obj *env) {
    (void)runtime;
    lisp_obj *obj = calloc(1, sizeof(lisp_obj));
    obj->kind = LOBJ_CONS;
    obj->params = params;
    obj->body = body;
    obj->env = env;
    return obj;
}

void
lisp_print(lisp_runtime *runtime, lisp_obj *obj) {
    assert(obj);

    switch (obj->kind) {
    default:
        assert(0);
    case LOBJ_CONS:
        printf("(");
        for (;;) {
            lisp_print(runtime, obj->car);
            if (obj->cdr == runtime->nil) {
                break;
            }

            if (obj->cdr->kind != LOBJ_CONS) {
                printf(" . ");
                lisp_print(runtime, obj->cdr);
                break;
            }

            printf(" ");
            obj = obj->cdr;
        }
        printf(")");
        break;
    case LOBJ_NUMI:
        printf("%lld", obj->numi);
        break;
    case LOBJ_SYMB:
        printf("%s", obj->symb);
        break;
    case LOBJ_NIL:
        printf("()");
        break;
    }
}
