#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Kinds of tokens that are result of lexing of lisp_lexer
 */
typedef enum {
    // Internal
    TOK_NONE = 0x0,
    // End of file
    TOK_EOF = 0x1,
    // Lisp symbol (identifier)
    TOK_SYMB = 0x2,
    // .
    TOK_DOT = 0x3,
    // (
    TOK_LPAREN = 0x4,
    // )
    TOK_RPAREN = 0x5,
    // "
    TOK_NUMI = 0x6,
    // 'adsada
    TOK_QUOTE = 0x7
} token_kind;

/**
 * @brief Struct storing data related to lexing lisp source code
 */
typedef struct {
    // Current parsing point
    char *cursor;
    // Buffer where strings & symbols are written as result of parsing
    char *tok_buf;
    // Size of tok_buf
    int tok_buf_capacity;
    // Kind of last_lexed token.
    token_kind kind;
    // Numeric value
    int64_t numi;
    // Length of written to tok_buf symbol
    uint32_t symb_len;
    // Internal, used for implementing 'peek' behaviour
    bool use_old;
} lisp_lexer;

/**
 * @brief Initializes lisp_lexer for working
 *
 * @param lex lexer to initialize
 * @param data data that needs to be lexed, lisp source code
 * @param tok_buf memory buffer, where strings accumulated while parsing will be
 * written
 * @param tok_buf_capacity size of tok_buf in bytes
 */
void lisp_lexer_init(lisp_lexer *lex, char *data, char *tok_buf,
                     uint32_t tok_buf_capacity);

/**
 * @brief 'Peeks' next token. If no token is currently present,
 * generates new. Otherwise does nothing until lexer_eat is called
 *
 * @param lex lexer
 *
 * @return false if resulting token kind is TOK_EOF
 */
bool lisp_lexer_peek(lisp_lexer *lex);

/**
 * @brief Tells lexer to generate next token upon next call of lisp_lexer_peek
 * instead of doing nothing
 *
 * @param lex lexer
 */
void lisp_lexer_eat(lisp_lexer *lex);

/**
 * @brief Does two consequetive calls to lisp_lexer_eat and lisp_lexer_peek.
 * Convenience function
 *
 * @param lex lexer
 */
void lisp_lexer_eat_peek(lisp_lexer *lex);

#endif
