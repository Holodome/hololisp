# hololisp 

[![codecov](https://codecov.io/gh/Holodome/hololisp/branch/master/graph/badge.svg?token=U41DRI0GU9)](https://codecov.io/gh/Holodome/hololisp)
[![Build and Test](https://github.com/Holodome/hololisp/actions/workflows/test.yml/badge.svg)](https://github.com/Holodome/hololisp/actions/workflows/test.yml)

Simple minimal lisp dialect runtime.

## Goals and design

The goal is to provide simple embeddable version of lisp that can be used as a playground for exploring various interprenter design and programming languages' ideas.
C was chosen because of its ease of use and integration into other systems.

The release version is shipped both as a library and executable. 

## Testing

Unit testing is done via super-simple self-written unit testing framework. For unit test info see Makefile.
System testing is done using Python.
