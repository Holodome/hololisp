#ifndef __HLL_TEST_H__
#define __HLL_TEST_H__

#include <stdio.h>

#define hlt_assertv(_msg, _test) \
    do {                         \
        if (!(_test)) {          \
            return (_msg);       \
        }                        \
    } while (0)

#define hlt_assert__(_test, _file, _line) \
    hlt_assertv("ASSERTION FAILED: " #_test " in " #_file ":" #_line, _test)
#define hlt_assert_(_test, _file, _line) hlt_assert__(_test, _file, _line)
#define hlt_assert(_test) hlt_assert_(_test, __FILE__, __LINE__)

#define hlt_run_test(_test)                                  \
    do {                                                     \
        char const *msg = _test();                           \
        ++hlt_tests_run;                                     \
        if (msg) {                                           \
            fprintf(stderr, "Function " #_test " failed\n"); \
            return msg;                                      \
        }                                                    \
    } while (0)

static int hlt_tests_run;

char const *all_tests();

int
main(void) {
    char const *result;
    printf("Running file %s:\n", __BASE_FILE__);
    result = all_tests();
    if (result != 0) {
        printf("FAIL: %s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", hlt_tests_run);

    return result != 0;
}

#endif
