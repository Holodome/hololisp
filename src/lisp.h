#ifndef __LISP_H__
#define __LISP_H__

#include <stdint.h>

/**
 * @brief Kinds of lisp objects
 */
typedef enum {
    // Internal, means unitialized data
    LOBJ_NONE,
    // Integer
    LOBJ_NUMI,
    // Cons
    LOBJ_CONS,
    // Symbol
    LOBJ_SYMB,
    // Function
    LOBJ_FUNC,
    // nil
    LOBJ_NIL,
    // t
    LOBJ_TRUE,
    // Builtin function
    LOBJ_BIND,
    // Environment frame
    LOBJ_ENV,
    // Macro definition
    LOBJ_MACRO,
} lisp_obj_kind;

struct lisp_runtime;

// Callback function prototype that can be used to call lisp functions that rely
// on c code
typedef struct lisp_obj *lisp_func_binding(struct lisp_runtime *,
                                           struct lisp_obj *);

/**
 * @brief Lisp object
 */
typedef struct lisp_obj {
    lisp_obj_kind kind;
    union {
        int64_t numi;
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
        lisp_func_binding *bind;
    };
} lisp_obj;

/**
 * @brief Struct containing all data needed to execute lisp code
 */
typedef struct lisp_runtime {
    // Lexer from where tokens needed for executing code will be generated
    struct lisp_lexer *lexer;
    // Stack of environment variables
    lisp_obj *env;
    // Nil object
    lisp_obj *nil;
    // Symbols
    lisp_obj *obarray;
} lisp_runtime;

/**
 * @brief Creates new lisp context
 *
 * @return Lisp context
 */
lisp_runtime *lisp_create(void);

/**
 * @brief Executes code from given text buffer
 *
 * @param runtime lisp runtime to use
 * @param buffer buffer from which to execute code
 * @param buffer_size size of buffer in bytes
 */
void lisp_exec_buffer(lisp_runtime *runtime, char *buffer);

/**
 * @brief Makes cons lisp object
 */
lisp_obj *lisp_make_cons(lisp_runtime *runtime, lisp_obj *car, lisp_obj *cdr);

/**
 * @brief Makes num lisp object
 */
lisp_obj *lisp_make_num(lisp_runtime *runtime, int64_t value);

/**
 * @brief Makes env lisp object
 */
lisp_obj *lisp_make_env(lisp_runtime *runtime, lisp_obj *up);

/**
 * @brief Makes symbol lisp object
 */
lisp_obj *lisp_make_symbol(lisp_runtime *runtime, char *symb);

/**
 * @brief Makes function lisp object
 */
lisp_obj *lisp_make_func(lisp_runtime *runtime, lisp_obj *params,
                         lisp_obj *body, lisp_obj *env);

lisp_obj *lisp_make_acons(lisp_runtime *runtime, lisp_obj *key, lisp_obj *datum,
                          lisp_obj *alist);

/**
 * @brief Prints given lisp object to stdout
 *
 * @param runtime lisp runtime
 * @param obj object to print
 */
void lisp_print(lisp_runtime *runtime, lisp_obj *obj);

void lisp_add_binding(lisp_runtime *runtime, char *name,
                      lisp_func_binding *bind);

lisp_obj *lisp_eval(lisp_runtime *runtime, lisp_obj *list);

#endif

