#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    PROGM_INTERP = 0x0,  // Typical interpretation, default mode
    PROGM_LEX    = 0x1,  // Only lexer, used for testing
    PROGM_PP     = 0x2,  // Run preprocessor for macro unwrapping
} program_mode;

typedef struct {
    char const **filenames;
    uint32_t     filename_count;
    program_mode mode;

    char const *output_name;
} options;

static void
help(uint32_t rc) {
    printf(
        "OVERVIEW: toy lisp interprenter\n"
        "\n"
        "USAGE: hololisp [options] file...\n");
    exit(rc);
}

static void
version(void) {
    printf("hololisp version 0.0.1\n");
    exit(0);
}

static options
cli_params(uint32_t argc, char **argv) {
    options options = {0};

    // First, collect files. This is not to use any dynamic allocations for
    // filename storage because it seems like an overkill
    uint32_t cursor = 1;

    // Now parse options
    cursor = 1;
    while (cursor < argc) {
        char const *option = argv[cursor++];

        if (!strcmp(option, "--version") || !strcmp(option, "-v")) {
            version();
        } else if (!strcmp(option, "--help") || !strcmp(option, "-h")) {
            help(0);
        } else if (!strcmp(option, "--lex")) {
            options.mode = PROGM_LEX;
        } else if (!strcmp(option, "--pp")) {
            options.mode = PROGM_PP;
        } else if (!strcmp(option, "--interp")) {
            options.mode = PROGM_INTERP;
        } else if (!strcmp(option, "--output") || !strcmp(option, "-o")) {
            if (cursor + 1 >= argc) {
                fprintf(stderr,
                        "argument to '-o' is missing (expected 1 value)\n");
            } else {
                options.output_name = argv[cursor++];
            }
        }
    }

    return options;
}

static int
execute(options options) {
    int rc = EXIT_SUCCESS;

    switch (options.mode) {
    default:
        assert(0);
    case PROGM_INTERP:
        break;
    case PROGM_LEX:
        break;
    case PROGM_PP:
        break;
    }

    return rc;
}

int
main(int argc, char **argv) {
    options options = cli_params(argc, argv);

    int rc = execute(options);

    return rc;
}
