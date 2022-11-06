# hololisp

[![codecov](https://codecov.io/gh/Holodome/hololisp/branch/master/graph/badge.svg?token=U41DRI0GU9)](https://codecov.io/gh/Holodome/hololisp)
[![Build and Test](https://github.com/Holodome/hololisp/actions/workflows/test.yml/badge.svg)](https://github.com/Holodome/hololisp/actions/workflows/test.yml)

[*hololisp*](https://holodome.github.io/) is a lisp-1 bytecode-compiled single-threaded small toy programming language.

## Features

*hololisp* takes inspiration from popular lisp dialects, primarily [scheme](https://en.wikipedia.org/wiki/Scheme_(programming_language)). It includes bytecode compiler and interpreter, along with tracing garbage collector. 

*hololisp* supports primitive data types:
* Numbers (internally represented as IEEE754 doubles)
* Conses
* Symbols

Language features include:
* Variables
* Closures
* Conditionals
* Builtin forms (+, =, list and others)
* User-defined functions
* C FFI
* Functions with variadic number of arguments
* Macros
* REPL

## Compile

```bash
make
```

hololisp is currently tested on arm64 macOS and Linux amd64. 

By default, hololisp is compiled in release mode. If you want to compile in debug mode add `DEBUG=1` flag to make.

## Testing

```bash
make test
```

hololisp testing is divided in two parts: unit testing and system-wide functional testing. Unit testing is done using [acutest](https://github.com/mity/acutest). System-wide testing is
done using small programs located in .sh files found in tests directory.

Code coverage can be collected if non-empty *COV* flag is passed to make:
```bash
make test DEBUG=1 COV=1
```

## Usage

*hololisp* can be used in three modes: REPL, executing script files, and executing command strings.

To execute script pass its name to the program executable:

```bash
hololisp examples/game_of_life.hl
```

REPL session looks like this:

```shell
$ hololisp
hololisp> (define x 100)
x
hololisp> x
100
hololisp> (+ x (* x 100))
10100
```

To execute command string pass it after *-e* flag:

```shell
$ hololisp -e "(+ 1 2)"
3
```

## Examples

```scheme
(defmacro (compare pred pivot)
  (list 'lambda (list 'it) (list pred 'it pivot)))

(define (qsort lis)
  (when lis
    (let ((pivot (car lis)) 
          (lis (cdr lis)))
      (append
        (qsort (filter (compare < pivot) lis))
        (list pivot)
        (qsort (filter (compare >= pivot) lis))))))

(print (qsort (list 5 6 2 1 4 5 2 6 3 9 1)))
; (1 1 2 2 3 4 5 5 6 6 9)
```

Examples can be found in the '*examples*' directory.

List of examples: 
* [Conway's Game of life](examples/game_of_life.hl)
* [Maze generator using recursive backtracking algorithm](examples/mazegen.hl)
* [quicksort](examples/quicksort.hl)

