#include "../src/lisp.h"
#include "../src/lisp_gcs.h"
#include "acutest.h"

static void
test_lisp_print_nil(void) {
}

static void
test_lisp_print_number(void) {
}

static void
test_lisp_print_symbol(void) {
}

static void
test_lisp_print_single_element_list(void) {
}

static void
test_lisp_print_list(void) {
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_lisp_print_nil), TCASE(test_lisp_print_number),
              TCASE(test_lisp_print_symbol),
              TCASE(test_lisp_print_single_element_list),
              TCASE(test_lisp_print_list) };
