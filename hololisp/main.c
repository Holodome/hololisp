#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_compiler.h"
#include "hll_hololisp.h"

#ifdef HLL_MEM_CHECK
#include "hll_mem.h"
#endif

typedef enum {
  HLL_MODE_EREPL,
  HLL_MODE_EREPL_NO_TTY,
  HLL_MODE_ESCRIPT,
  HLL_MODE_ESTRING,
  HLL_MODE_HELP,
  HLL_MODE_VERSION,
  HLL_MODE_DUMP_BYTECODE,
} hll_mode;

typedef struct {
  hll_mode mode;
  const char *str;
  bool forbid_colors;
} hll_options;

static char *read_entire_file(const char *filename) {
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
             "  -h        Show this message\n"
             "  -m        Do not use colored output\n");
}

static void print_version(void) { printf("hololisp 1.0.0\n"); }

static bool parse_cli_args(hll_options *opts, uint32_t argc,
                           const char **argv) {
  uint32_t cursor = 0;
  while (cursor < argc) {
    const char *opt = argv[cursor++];
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
    } else if (strcmp(opt, "--dump") == 0) {
      opts->mode = HLL_MODE_DUMP_BYTECODE;
    } else if (strcmp(opt, "-m") == 0) {
      opts->forbid_colors = true;
    } else {
      fprintf(stderr, "Unknown option '%s'\n", opt);
      print_usage(stderr);
      return true;
    }
  }

  return false;
}

static bool execute_repl(hll_options *opts, bool tty) {
  bool result = false;
  struct hll_vm *vm = hll_make_vm(NULL);

  for (;;) {
    if (tty) {
      printf("hololisp> ");
    }
    char line[4096];
    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }

    hll_interpret_flags flags = HLL_INTERPRET_PRINT_RESULT;
    if (!opts->forbid_colors) {
      flags |= HLL_INTERPRET_DEBUG_COLORED;
    }
    hll_interpret_result interpret_result =
        hll_interpret(vm, line, "repl", flags);
    result = interpret_result == HLL_RESULT_ERROR;
  }

  hll_delete_vm(vm);

  return result;
}

static bool execute_script(hll_options *opts) {
  if (opts->str == NULL) {
    fprintf(stderr, "No filename provided\n");
    return true;
  }

  char *file_contents = read_entire_file(opts->str);
  if (file_contents == NULL) {
    fprintf(stderr, "failed to read file '%s'\n", opts->str);
    return true;
  }

  struct hll_vm *vm = hll_make_vm(NULL);
  hll_interpret_flags flags = 0;
  if (!opts->forbid_colors) {
    flags |= HLL_INTERPRET_DEBUG_COLORED;
  }
  hll_interpret_result interpret_result =
      hll_interpret(vm, file_contents, opts->str, flags);
  bool result = interpret_result == HLL_RESULT_ERROR;
  hll_delete_vm(vm);
  free(file_contents);

  return result;
}

static bool execute_string(hll_options *opts) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_interpret_flags flags = HLL_INTERPRET_PRINT_RESULT;
  if (!opts->forbid_colors) {
    flags |= HLL_INTERPRET_DEBUG_COLORED;
  }
  hll_interpret_result result = hll_interpret(vm, opts->str, "cli", flags);
  hll_delete_vm(vm);

  return result == HLL_RESULT_ERROR;
}

static bool execute_dump_bytecode(const char *filename) {
  if (filename == NULL) {
    fprintf(stderr, "No filename provided\n");
    return true;
  }

  char *file_contents = read_entire_file(filename);
  if (file_contents == NULL) {
    fprintf(stderr, "failed to read file '%s'\n", filename);
    return true;
  }

  struct hll_vm *vm = hll_make_vm(NULL);
  hll_value compiled;
  if (!hll_compile(vm, file_contents, filename, &compiled)) {
    return false;
  }

  hll_dump_program_info(stdout, compiled);
  free(file_contents);

  return false;
}

static bool execute(hll_options *opts) {
  bool error = false;
  switch (opts->mode) {
  case HLL_MODE_DUMP_BYTECODE:
    error = execute_dump_bytecode(opts->str);
    break;
  case HLL_MODE_EREPL:
    error = execute_repl(opts, true);
    break;
  case HLL_MODE_EREPL_NO_TTY:
    error = execute_repl(opts, false);
    break;
  case HLL_MODE_ESCRIPT:
    error = execute_script(opts);
    break;
  case HLL_MODE_ESTRING:
    error = execute_string(opts);
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

int main(int argc, const char **argv) {
  int result = EXIT_SUCCESS;
  hll_options opts = {0};
  if (parse_cli_args(&opts, argc - 1, argv + 1)) {
    result = EXIT_FAILURE;
    goto out;
  }

  if (opts.mode == HLL_MODE_EREPL && !HLL_IS_STDIN_A_TTY) {
    opts.mode = HLL_MODE_EREPL_NO_TTY;
  }

  if (execute(&opts)) {
    result = EXIT_FAILURE;
    goto out;
  }

out:
  (void)0;
#if HLL_MEM_CHECK
  size_t not_freed = hll_mem_check();
  if (not_freed) {
    fprintf(stderr, "Memory check failed: %zu not freed!\n", not_freed);
    result = EXIT_FAILURE;
  }
#endif
  return result;
}
