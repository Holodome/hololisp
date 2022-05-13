#ifndef __HLL_TEST_H__
#define __HLL_TEST_H__

#include <stdio.h>

#define hlt_assertv(_msg, _test) \
    do {                         \
        if (!(_test)) {          \
            return (_msg);       \
        }                        \
    } while (0)

#define hlt_assert(_test) hlt_assertv("ASSERTION FAILED: " #_test, _test)

#define hlt_run_test(_test)        \
    do {                           \
        char const *msg = _test(); \
        ++hlt_tests_run;           \
        if (msg) {                 \
            return msg;            \
        }                          \
    } while (0)

static int hlt_tests_run;

char const *all_tests();

int
main(void) {
    char const *result = all_tests();
    if (result != 0) {
        printf("FAIL: %s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", hlt_tests_run);

    return result != 0;
}

#endif
