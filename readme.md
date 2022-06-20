#hololisp

[![codecov](https://codecov.io/gh/Holodome/hololisp/branch/master/graph/badge.svg?token=U41DRI0GU9)](https://codecov.io/gh/Holodome/hololisp)
[![Build and Test](https://github.com/Holodome/hololisp/actions/workflows/test.yml/badge.svg)](https://github.com/Holodome/hololisp/actions/workflows/test.yml)

Simple minimal lisp dialect runtime.

## Goals and design

The goal is to provide simple embeddable version of lisp that can be used as a playground for exploring various interprenter design and programming languages' ideas.
C was chosen because of its ease of use and integration into other systems.

One of the most important points in designing this project is simplicity. It is easy to get overwhelmed by features that are usually implemented in other lisp dialects. These may include JIT, static compilation, different kinds of memory managment techniques, code optimization and others. It has been decided that *hololisp* should support only minimal possible features that allow for its usage.

To clarify, this means that we do only interpretation of the source code but with optimization of tail calls and does not support any kind of compilation.

## Features 

hololisp implements some of the common standard functions and features found in most popular lisp dialects, like [Common Lisp](https://common-lisp.net/) and [Scheme](https://www.scheme.com/tspl4/).

This lisp implementation supports basic data types:
* Integers
* Conses
* Symbols

Language features include:
* Global variables
* Lexically-scoped variables
* Closures
* *if* conditionals
* Primitive functions (+, =, list and others)
* User-defined functions

## Compile

```bash
make
```

hololisp is currently tested on arm64 MacOS and Linux amd64. The code uses some POSIX functions, so it is not portable to Windows without modification.

## Testing

```bash
make test
```

hololisp testing is divided in two parts: unit testing and system-wide functional testing. Unit testing is done using [acutest](https://github.com/mity/acutest). System-wide testing is
done is .sh files found in tests directory.

Code coverage can be collected if non-empty *COV* flag is passed to make:
```bash
make test COV=1
```

## Usage

Program can be run in two modes: Interactive REPL traditional to lisp interprenters and executing scripts.
To execute script pass its name to the program executable:

```bash
hololisp examples/game_of_life.lisp
```

Usual REPL session looks like this:

```shell
$ hololisp
hololisp> (defvar x 100)
x
hololisp> x
100
hololisp> (+ x (* x 100))
10100
```

Examples can be found in the *examples* folder.

