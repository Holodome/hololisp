#include "lisp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

static lisp_obj *nil = &(lisp_obj){.kind = LOBJ_NIL};

static lisp_obj *read_expr(lisp_runtime *runtime);

static lisp_obj *
find_symb(lisp_runtime *runtime, char *symb) {
    lisp_obj *obj;
    for (obj = runtime->obarray; obj != nil; obj = obj->cdr) {
        if (strcmp(obj->car->symb, symb) == 0) {
            break;
        }
    }

    if (obj == nil) {
        obj = lisp_make_symbol(runtime, symb);
        runtime->obarray = lisp_make_cons(runtime, obj, runtime->obarray);
    } else {
        obj = obj->car;
    }
    return obj;
}

static lisp_obj *
reverse_list(lisp_obj *obj) {
    lisp_obj *ret = nil;
    while (obj != nil) {
        lisp_obj *head = obj;
        obj = obj->cdr;
        head->cdr = ret;
        ret = head;
    }
    return ret;
}

static lisp_obj *
read_cons(lisp_runtime *runtime) {
    lisp_obj *expr = nil;

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
            expr = reverse_list(expr);
            break;
        } else if (runtime->lexer->kind == TOK_DOT) {
            lisp_lexer_eat_peek(runtime->lexer);
            lisp_obj *car = read_expr(runtime);
            expr = reverse_list(lisp_make_cons(runtime, car, expr));
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

static lisp_obj *
read_quote(lisp_runtime *runtime) {
    lisp_obj *sym = find_symb(runtime, "quote");
    lisp_lexer_eat(runtime->lexer);
    return lisp_make_cons(runtime, sym,
                          lisp_make_cons(runtime, read_expr(runtime), nil));
}

static lisp_obj *
read_expr(lisp_runtime *runtime) {
    lisp_obj *expr = nil;

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

static uint64_t
list_length(lisp_obj *list) {
    uint64_t result = 0;
    for (; list != nil; list = list->cdr) {
        ++result;
    }
    return result;
}

static lisp_obj *
builtin_print(lisp_runtime *runtime, lisp_obj *list) {
    lisp_print(runtime, lisp_eval(runtime, list->car));
    printf("\n");
    return nil;
}

static lisp_obj *
builtin_add(lisp_runtime *runtime, lisp_obj *list) {
    int64_t result = 0;
    for (lisp_obj *obj = list; obj != nil; obj = obj->cdr) {
        result += lisp_eval(runtime, obj->car)->numi;
    }
    return lisp_make_num(runtime, result);
}

static lisp_obj *
builtin_sub(lisp_runtime *runtime, lisp_obj *list) {
    int64_t result = lisp_eval(runtime, list->car)->numi;
    list = list->cdr;
    for (; list != nil; list = list->cdr) {
        result -= lisp_eval(runtime, list->car)->numi;
    }
    return lisp_make_num(runtime, result);
}

static lisp_obj *
builtin_mul(lisp_runtime *runtime, lisp_obj *list) {
    int64_t result = 1;
    for (lisp_obj *obj = list; obj != nil; obj = obj->cdr) {
        result *= lisp_eval(runtime, obj->car)->numi;
    }
    return lisp_make_num(runtime, result);
}

static lisp_obj *
builtin_div(lisp_runtime *runtime, lisp_obj *list) {
    int64_t result = lisp_eval(runtime, list->car)->numi;
    list = list->cdr;
    for (; list != nil; list = list->cdr) {
        result /= lisp_eval(runtime, list->car)->numi;
    }
    return lisp_make_num(runtime, result);
}

static lisp_obj *
builtin_defvar(lisp_runtime *runtime, lisp_obj *list) {
    assert(list_length(list) == 2);
    assert(list->car->kind == LOBJ_SYMB);
    lisp_obj *symb = list->car;
    lisp_obj *value = lisp_eval(runtime, list->cdr->car);
    runtime->env->vars =
        lisp_make_acons(runtime, symb, value, runtime->env->vars);
    return value;
}

static lisp_obj *
builtin_setq(lisp_runtime *runtime, lisp_obj *list) {
    assert(list_length(list) == 2);
    assert(list->car->kind == LOBJ_SYMB);
    lisp_obj *bind = lisp_find(runtime, list->car);
    assert(bind);
    lisp_obj *value = lisp_eval(runtime, list->cdr->car);
    bind->cdr = value;
    return value;
}

static lisp_obj *
builtin_defun(lisp_runtime *runtime, lisp_obj *list) {
    lisp_obj *symb = list->car;
    assert(symb->kind == LOBJ_SYMB);
    lisp_obj *args = list->cdr->car;
    assert(args->kind == LOBJ_CONS);
    for (lisp_obj *test = args; test != nil; test = test->cdr) {
        assert(test->car->kind == LOBJ_SYMB);
        assert(test->cdr->kind == LOBJ_NIL || test->cdr->kind == LOBJ_CONS);
    }
    lisp_obj *body = list->cdr->cdr->car;

    lisp_obj *func = lisp_make_func(runtime, args, body, runtime->env);
    runtime->env->vars =
        lisp_make_acons(runtime, symb, func, runtime->env->vars);
    return func;
}

lisp_runtime *
lisp_create(void) {
    lisp_runtime *runtime = calloc(1, sizeof(lisp_runtime));

    lisp_obj *env = lisp_make_env(runtime, nil);
    runtime->env = env;

    runtime->lexer = calloc(1, sizeof(lisp_lexer));
    runtime->obarray = nil;

    lisp_add_binding(runtime, "print", builtin_print);
    lisp_add_binding(runtime, "+", builtin_add);
    lisp_add_binding(runtime, "*", builtin_mul);
    lisp_add_binding(runtime, "/", builtin_div);
    lisp_add_binding(runtime, "-", builtin_sub);
    lisp_add_binding(runtime, "setq", builtin_setq);
    lisp_add_binding(runtime, "defvar", builtin_defvar);
    lisp_add_binding(runtime, "defun", builtin_defun);

    return runtime;
}

void
lisp_exec_buffer(lisp_runtime *runtime, char *buffer) {
    char tok_buf[4096];
    lisp_lexer_init(runtime->lexer, buffer, tok_buf, sizeof(tok_buf));

    while (lisp_lexer_peek(runtime->lexer)) {
        lisp_obj *cons = read_expr(runtime);
        lisp_eval(runtime, cons);
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
    obj->vars = nil;
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
    obj->kind = LOBJ_FUNC;
    obj->params = params;
    obj->body = body;
    obj->env = env;
    return obj;
}

lisp_obj *
lisp_make_acons(lisp_runtime *runtime, lisp_obj *key, lisp_obj *datum,
                lisp_obj *alist) {
    return lisp_make_cons(runtime, lisp_make_cons(runtime, key, datum), alist);
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
            if (obj->cdr == nil) {
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

void
lisp_add_binding(lisp_runtime *runtime, char *symb, lisp_func_binding *bind) {
    lisp_obj *binding = calloc(1, sizeof(lisp_obj));
    binding->kind = LOBJ_BIND;
    binding->bind = bind;

    lisp_obj *symb_obj = find_symb(runtime, symb);
    runtime->env->vars =
        lisp_make_acons(runtime, symb_obj, binding, runtime->env->vars);
}

lisp_obj *
lisp_find(lisp_runtime *runtime, lisp_obj *obj) {
    lisp_obj *result = 0;
    for (lisp_obj *env = runtime->env; env != nil && !result; env = env->up) {
        for (lisp_obj *test = env->vars; test != nil && !result;
             test = test->cdr) {
            if (test->car->car == obj) {
                result = test->car;
            }
        }
    }

    return result;
}

static lisp_obj *
lisp_progn(lisp_runtime *runtime, lisp_obj *list) {
    lisp_obj *result = nil;
    for (; list != nil; list = list->cdr) {
        result = lisp_eval(runtime, list->car);
    }
    return result;
}

static lisp_obj *
apply_func(lisp_runtime *runtime, lisp_obj *fn, lisp_obj *args) {
    lisp_obj *result;
    switch (fn->kind) {
    default:
        assert(0);
    case LOBJ_BIND:
        result = fn->bind(runtime, args);
        break;
    case LOBJ_FUNC: {
        lisp_obj *stack_env = runtime->env;
        lisp_obj *new_env = lisp_make_env(runtime, fn->env);
        lisp_obj *arg_names = fn->params;
        for (; args != nil; args = args->cdr, arg_names = arg_names->cdr) {
            new_env->vars =
                lisp_make_acons(runtime, arg_names->car, args->car, new_env->vars);
        }
        runtime->env = new_env;
        result = lisp_progn(runtime, fn->body);
        runtime->env = stack_env;
    } break;
    }
    return result;
}

lisp_obj *
lisp_eval(lisp_runtime *runtime, lisp_obj *list) {
    lisp_obj *result = nil;
    switch (list->kind) {
    default:
        assert(0);
    case LOBJ_BIND:
    case LOBJ_FUNC:
    case LOBJ_NUMI:
        result = list;
        break;
    case LOBJ_SYMB: {
        lisp_obj *bind = lisp_find(runtime, list);
        if (!bind) {
            fprintf(stderr, "Use of undeclared symbol '%s'\n", list->symb);
        }
        assert(bind);
        result = bind->cdr;
    } break;
    case LOBJ_CONS: {
        lisp_obj *fn = lisp_eval(runtime, list->car);
        lisp_obj *args = list->cdr;
        result = apply_func(runtime, fn, args);
    } break;
    }
    return result;
}
