#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMB_CHARS "~!@#$%^&*-_=+:/?<>"

enum {
    TOK_NONE,
    TOK_EOF,
    TOK_SYMB,
    TOK_DOT,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_NUMI,
    TOK_QUOTE
};

typedef struct {
    char *cursor;

    char *tok_buf;
    int tok_buf_capacity;

    int kind;
    char punct;
    long long numi;
    int symb_len;

    bool use_old;
} lexer;

lexer
make_lexer(char *data, char *tok_buf, int tok_buf_capacity) {
    lexer lex = {.cursor           = data,
                 .tok_buf          = tok_buf,
                 .tok_buf_capacity = tok_buf_capacity};
    return lex;
}

bool
lexer_peek(lexer *lex) {
    if (lex->use_old) {
        goto out;
    }

    lex->use_old = true;
    lex->kind    = TOK_NONE;
    while (lex->kind == TOK_NONE) {
        char cp = *lex->cursor;
        if (!cp) {
            lex->kind = TOK_EOF;
            continue;
        }

        if (isspace(cp)) {
            do {
                cp = *++lex->cursor;
            } while (cp && isspace(cp));
            continue;
        }

        if (cp == ';') {
            do {
                cp = *++lex->cursor;
            } while (cp && cp != '\n');

            if (cp == '\n') {
                cp = *++lex->cursor;
            }
            continue;
        }

        if (cp == '\'') {
            ++lex->cursor;
            lex->kind = TOK_QUOTE;
            continue;
        }

        if (cp == '.') {
            ++lex->cursor;
            lex->kind = TOK_DOT;
            continue;
        }

        if (cp == '(') {
            ++lex->cursor;
            lex->kind = TOK_LPAREN;
            continue;
        }

        if (cp == ')') {
            ++lex->cursor;
            lex->kind = TOK_RPAREN;
            continue;
        }

        char *write_cursor = lex->tok_buf;
        char *write_eof    = lex->tok_buf + lex->tok_buf_capacity;

        *write_cursor++ = cp;
        cp              = *++lex->cursor;

        while (cp && (isalnum(cp) || strchr(SYMB_CHARS, cp))) {
            if (write_cursor < write_eof) {
                *write_cursor++ = cp;
            }
            cp = *++lex->cursor;
        }

        *write_cursor = 0;
        lex->symb_len = write_cursor - lex->tok_buf;
        lex->kind     = TOK_SYMB;
        continue;
    }

out:
    return lex->kind != TOK_EOF;
}

void
lexer_eat(lexer *lex) {
    lex->use_old = false;
}

void
lexer_eat_peek(lexer *lex) {
    lexer_eat(lex);
    lexer_peek(lex);
}

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
        long long numi;
        struct {
            struct lisp_obj *car;
            struct lisp_obj *cdr;
        };
        struct {
            struct lisp_obj *params;
            struct lisp_obj *body;
            struct lisp_obj *env;
        };
        struct {
            struct lisp_obj *vars;
            struct lisp_obj *up;
        };
        char symb[1];
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

lisp_obj *eval(lisp_runtime *rt, lisp_obj *obj);

int
list_length(lisp_runtime *rt, lisp_obj *obj) {
    int r = 0;
    for (;;) {
        if (obj == rt->nil) {
            break;
        }

        assert(obj->kind == OBJ_CONS);
        obj = obj->cdr;
        ++r;
    }
    return r;
}

lisp_obj *
progn(lisp_runtime *rt, lisp_obj *obj) {
    lisp_obj *r = rt->nil;
    assert(obj->kind == OBJ_CONS);
    for (lisp_obj *temp = obj; temp != rt->nil; temp = temp->cdr) {
        assert(temp->kind == OBJ_CONS);
        r = eval(rt, temp->car);
    }
    return r;
}

lisp_obj *
find(lisp_runtime *rt, lisp_obj *symb) {
    lisp_obj *r = 0;
    for (lisp_obj *env = rt->locals; env; env = env->up) {
        for (lisp_obj *cons = env->vars; cons != rt->nil; cons = cons->cdr) {
            lisp_obj *bind = cons->car;
            if (bind->car == symb) {
                r = bind;
                goto out;
            }
        }
    }
out:
    return r;
}

lisp_obj *
push_env(lisp_runtime *rt, lisp_obj *vars, lisp_obj *vals) {
    assert(list_length(rt, vars) == list_length(rt, vals));
    lisp_obj *map = rt->nil;
    for (lisp_obj *var = vars, *val = vals; var != rt->nil;
         var = var->cdr, val = val->cdr) {
        rt->locals = make_cons(make_cons(var->car, val->car), map);
    }

    lisp_obj *env = calloc(1, sizeof(lisp_obj));
    env->kind     = OBJ_ENV;
    env->up       = rt->locals;
    return env;
}

lisp_obj *
macroexpand(lisp_runtime *rt, lisp_obj *obj) {
    lisp_obj *r = obj;
    if (obj->kind == OBJ_CONS && obj->car->kind == OBJ_SYMB) {
        lisp_obj *bind = find(rt, obj->car);
        if (bind && bind->cdr->kind == OBJ_MACRO) {
            lisp_obj *args   = obj->cdr;
            lisp_obj *body   = bind->cdr->body;
            lisp_obj *params = bind->cdr->params;
            lisp_obj *newenv = push_env(rt, params, args);

            lisp_obj *oldenv = rt->locals;
            rt->locals       = newenv;
            r                = progn(rt, body);
            rt->locals       = oldenv;
        }
    }
    return r;
}

lisp_obj *
eval_list(lisp_runtime *rt, lisp_obj *list) {
    lisp_obj *head = 0, *tail = 0;
    for (lisp_obj *iter = list; iter != rt->nil; iter = iter->cdr) {
        lisp_obj *temp = eval(rt, iter->car);
        if (!head) {
            head = tail = make_cons(temp, rt->nil);
        } else {
            tail->cdr = make_cons(temp, rt->nil);
            tail = tail->cdr;
        }
    }

    lisp_obj *result = rt->nil;
    if (head) {
        result = head;
    }
    return head;
}

bool
is_list(lisp_obj *obj) {
    return obj->kind == OBJ_NIL || obj->kind == OBJ_CONS;
}

lisp_obj *apply(lisp_runtime *rt, lisp_obj *fn, lisp_obj *args) {
    assert(is_list(args));
    lisp_obj *r;
    if (fn->kind == OBJ_PRIMITIVE) {
        r = fn->prim(rt, args);
    } else if (fn->kind == OBJ_FUNC) {
        lisp_obj *body = fn->body;
        lisp_obj *env = calloc(1, sizeof(lisp_obj));
        env->kind     = OBJ_ENV;
        env->up       = rt->locals;
    }
}

lisp_obj *
eval(lisp_runtime *rt, lisp_obj *obj) {
    lisp_obj *r = rt->nil;
    switch (obj->kind) {
    default:
        assert(false);
    case OBJ_NUMI:
    case OBJ_FUNC:
        r = obj;
        break;
    case OBJ_SYMB:
        r = find(rt, obj);
        assert(r);
        break;
    case OBJ_CONS: {
        lisp_obj *expanded = macroexpand(rt, obj);
        if (expanded != obj) {
            r = eval(rt, expanded);
        } else {
            lisp_obj *fn   = eval(rt, obj->car);
            lisp_obj *args = obj->cdr;
            assert(fn->kind == OBJ_FUNC);
            r = apply_func(rt, fn, args);
        }
    } break;
    }
    return r;
}

lisp_obj *
prim_quote(lisp_runtime *rt, lisp_obj *obj) {
    assert(list_length(rt, obj) == 1);
    return obj->car;
}

lisp_obj *
prim_if(lisp_runtime *rt, lisp_obj *obj) {
    assert(list_length(rt, obj) >= 2);
    lisp_obj *result = rt->nil;

    lisp_obj *cond = eval(rt, obj->car);
    if (cond != rt->nil) {
        lisp_obj *then = obj->cdr->car;
        result         = then;
    } else {
        lisp_obj *else_ = obj->cdr->cdr;
        result          = else_ == rt->nil ? rt->nil : progn(rt, else_);
    }

    return result;
}

void
add_primitive(lisp_runtime *rt, char *name, primitive_func *func) {
    lisp_obj *prim = calloc(1, sizeof(lisp_obj));
    prim->kind     = OBJ_PRIMITIVE;
    prim->prim     = func;

    lisp_obj *symb = intern(rt, name);
    rt->locals     = make_cons(make_cons(symb, prim), rt->locals->vars);
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
