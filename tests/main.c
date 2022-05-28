#include <check.h>
#include <stdlib.h>

#include "../src/logger.h"

Suite* create_lexer_suite(void);

int
main(void) {
    int success = 0;
    SRunner *runner;
    Suite *lexer_suite = create_lexer_suite();

    runner = srunner_create(lexer_suite);
    srunner_run_all(runner, CK_VERBOSE);
    success = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (success == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
