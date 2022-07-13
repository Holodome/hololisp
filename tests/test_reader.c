#include "../hololisp/hll_compiler.h"
#include "acutest.h"

static hll_memory_arena arena = { 0 };

static void
test_reader_reports_eof(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_NIL);
}

static void
test_reader_parses_num(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "123", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);

    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_INT);
    TEST_ASSERT(ast->as.num == 123);
}

static void
test_reader_parses_symbol(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "hello-world", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_SYMB);
    TEST_ASSERT(strcmp(ast->as.symb, "hello-world") == 0);
}

static void
test_reader_parses_one_element_list(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(100)", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_INT);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
}

static void
test_reader_parses_list(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(100 -100 abc)", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_INT);
    TEST_ASSERT(ast->as.cons.cdr->kind != HLL_AST_NIL);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_INT);
    TEST_ASSERT(ast->as.cons.cdr->kind != HLL_AST_NIL);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
}

static void
test_reader_parses_nested_lists(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(+ (* 3 2) hello)", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;

    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_CONS);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_CONS);
    {
        hll_ast *inner = ast->as.cons.car;
        TEST_ASSERT(inner->as.cons.car->kind == HLL_AST_SYMB);
        TEST_ASSERT(inner->as.cons.cdr->kind == HLL_AST_CONS);
        inner = inner->as.cons.cdr;
        TEST_ASSERT(inner->as.cons.car->kind == HLL_AST_INT);
        TEST_ASSERT(inner->as.cons.cdr->kind == HLL_AST_CONS);
        inner = inner->as.cons.cdr;
        TEST_ASSERT(inner->as.cons.car->kind == HLL_AST_INT);
        TEST_ASSERT(inner->as.cons.cdr->kind == HLL_AST_NIL);
    }
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
}

static void
test_reader_reports_unclosed_list(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    (void)ast;
    TEST_ASSERT(reader.has_errors);
}

static void
test_reader_reports_stray_rparen(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, ")", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    (void)ast;
    TEST_ASSERT(reader.has_errors);
}

static void
test_reader_parses_nil(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "()", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_NIL);
}

static void
test_reader_parses_simple_dotted_cons(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(abc . 123)", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_INT);
}

static void
test_reader_parses_dotted_list(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "(a b c . 123)", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind != HLL_AST_NIL);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind != HLL_AST_NIL);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_INT);
}

static void
test_reader_parses_quote(void) {
    hll_lexer lexer;
    hll_lexer_init(&lexer, "'1", NULL);
    hll_reader reader;
    hll_reader_init(&reader, &lexer, &arena, NULL);

    hll_ast *ast = hll_read_ast(&reader);
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
    ast = ast->as.cons.car;
    TEST_ASSERT(ast->kind == HLL_AST_CONS);
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_SYMB);
    TEST_ASSERT(ast->as.cons.cdr->kind != HLL_AST_NIL);
    ast = ast->as.cons.cdr;
    TEST_ASSERT(ast->as.cons.car->kind == HLL_AST_INT);
    TEST_ASSERT(ast->as.cons.cdr->kind == HLL_AST_NIL);
}

#define TCASE(_name)  \
    {                 \
#_name, _name \
    }

TEST_LIST = { TCASE(test_reader_reports_eof),
              TCASE(test_reader_parses_num),
              TCASE(test_reader_parses_symbol),
              TCASE(test_reader_parses_one_element_list),
              TCASE(test_reader_parses_list),
              TCASE(test_reader_parses_nested_lists),
              TCASE(test_reader_reports_unclosed_list),
              TCASE(test_reader_reports_stray_rparen),
              TCASE(test_reader_parses_nil),
              TCASE(test_reader_parses_simple_dotted_cons),
              TCASE(test_reader_parses_dotted_list),
              TCASE(test_reader_parses_quote),
              { NULL, NULL } };
