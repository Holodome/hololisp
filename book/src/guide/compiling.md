# Compiling

hololisp is written in C99 and uses Make as its build system.

For basic usage just run `make` command and start using build/hololisp as language interpreter.

## Make flags

| Name of the option | Description                                           |
|--------------------|-------------------------------------------------------|
| DEBUG              | Specifies whether to compile in debug or release mode |
| COV                | Collect coverage (for use with gprof)                 |
| ASAN               | Compile with Address Sanitizer                        |
| STRESS_GC          | Run Garbage Collector on every allocation             |
