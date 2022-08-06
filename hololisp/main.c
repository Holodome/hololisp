#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_hololisp.h"

typedef enum {
  HLL_MODE_EREPL,
  HLL_MODE_EREPL_NO_TTY,
  HLL_MODE_ESCRIPT,
  HLL_MODE_ESTRING,
  HLL_MODE_HELP,
  HLL_MODE_VERSION
} hll_mode;

typedef struct {
  hll_mode mode;
  char const *str;
} hll_options;

static char *read_entire_file(char const *filename) {
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    return NULL;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }

  int file_size = ftell(f);
  if (file_size < 0) {
    fclose(f);
    return NULL;
  }

  rewind(f);

  char *buffer = malloc(file_size + 1);
  if (buffer == NULL) {
    fclose(f);
    return NULL;
  }

  if (fread(buffer, file_size, 1, f) != 1) {
    free(buffer);
    fclose(f);
    return NULL;
  }

  buffer[file_size] = '\0';

  if (fclose(f) != 0) {
    free(buffer);
    return NULL;
  }

  return buffer;
}

static void print_usage(FILE *f) {
  fprintf(f, "usage: hololisp [options] [script]\n"
             "Available options are:\n"
             "  -e stat   Execute string 'stat'\n"
             "  -v        Show version information\n"
             "  -h        Show this message\n");
}

static void print_version(void) { printf("hololisp 1.0.0\n"); }

static bool parse_cli_args(hll_options *opts, uint32_t argc,
                           char const **argv) {
  uint32_t cursor = 0;
  while (cursor < argc) {
    char const *opt = argv[cursor++];
    assert(opt != NULL);
    if (*opt != '-') {
      opts->mode = HLL_MODE_ESCRIPT;
      opts->str = opt;
      continue;
    }

    if (strcmp(opt, "-e") == 0) {
      if (cursor >= argc) {
        fprintf(stderr, "-e expects 1 parameter\n");
        return true;
      } else {
        opts->mode = HLL_MODE_ESTRING;
        opts->str = argv[cursor++];
      }
    } else if (strcmp(opt, "-v") == 0) {
      opts->mode = HLL_MODE_VERSION;
    } else if (strcmp(opt, "-h") == 0) {
      opts->mode = HLL_MODE_HELP;
    } else {
      fprintf(stderr, "Unknown option '%s'\n", opt);
      print_usage(stderr);
      return true;
    }
  }

  return false;
}

static bool manage_result(hll_interpret_result result) {
  bool error = false;
  switch (result) {
  case HLL_RESULT_COMPILE_ERROR:
    fprintf(stderr, "Compile error\n");
    error = true;
    break;
  case HLL_RESULT_RUNTIME_ERROR:
    fprintf(stderr, "Runtime error\n");
    error = true;
    break;
  case HLL_RESULT_OK:
    break;
  }

  return error;
}

static bool execute_repl(bool tty) {
  struct hll_vm *vm = hll_make_vm(NULL);

  for (;;) {
    if (tty) {
      printf("hololisp> ");
    }
    char line[4096];
    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }

    hll_interpret_result result = hll_interpret(vm, "repl", line);
    manage_result(result);
  }

  hll_delete_vm(vm);

  return false;
}

static bool execute_script(char const *filename) {
  if (filename == NULL) {
    fprintf(stderr, "No filename provided");
    return true;
  }

  char *file_contents = read_entire_file(filename);
  if (file_contents == NULL) {
    fprintf(stderr, "failed to read file '%s'\n", filename);
    return true;
  }

  struct hll_vm *vm = hll_make_vm(NULL);
  hll_interpret_result result = hll_interpret(vm, filename, file_contents);
  hll_delete_vm(vm);
  free(file_contents);

  return manage_result(result);
}

static bool execute_string(char const *str) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_interpret_result result = hll_interpret(vm, "cli", str);
  hll_delete_vm(vm);

  return manage_result(result);
}

static bool execute(hll_options *opts) {
  bool error = false;
  switch (opts->mode) {
  case HLL_MODE_EREPL:
    error = execute_repl(true);
    break;
  case HLL_MODE_EREPL_NO_TTY:
    error = execute_repl(false);
    break;
  case HLL_MODE_ESCRIPT:
    error = execute_script(opts->str);
    break;
  case HLL_MODE_ESTRING:
    error = execute_string(opts->str);
    break;
  case HLL_MODE_HELP:
    print_usage(stdout);
    break;
  case HLL_MODE_VERSION:
    print_version();
    break;
  }

  return error;
}

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include <unistd.h>
#define HLL_IS_STDIN_A_TTY isatty(STDIN_FILENO)

#elif defined(_WIN32)

#include <io.h>
#include <windows.h>

#define HLL_IS_STDIN_A_TTY _isatty(_fileno(stdin))

#else

#define HLL_IS_STDIN_A_TTY 1

#endif

int main(int argc, char const **argv) {
  hll_options opts = {0};
  if (parse_cli_args(&opts, argc - 1, argv + 1)) {
    return EXIT_FAILURE;
  }

  if (opts.mode == HLL_MODE_EREPL && !HLL_IS_STDIN_A_TTY) {
    opts.mode = HLL_MODE_EREPL_NO_TTY;
  }

  if (execute(&opts)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
