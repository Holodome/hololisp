# Design

> hololisp is a toy programming language. It does not have to perform well or use readably and user-friendly syntax as long as it is relatively fun to use and program.

## Background and motivation

It is common knowledge in programming languages' community that lisp is one of the simplest languages to implement while proving wide range of features. There are a lot of tutorials on the internet that describe how to implement simple lisp dialect interpreter.

All those tutorials, however, do not cover important topics of writing interpreter or cover them partially. This may include garbage collector, tail call unwinding, bytecode and other interesting topics. And, most importantly, they do not try to provide programmer (student) with sense of accomplishment that can be acquired after long hours spent on debugging malformed algorithms, optimizing naive code and spending more time fighting premature optimizations and improving on architectural imperfections.

This project does not try to label itself as a great learning material, but it does go a bit deeper into semi-complex PL topics. Everything is written from scratch without using any libraries mostly to keep things simple and to show that no complexity is ever necessary even when writing seemingly complex application.

## Language for implementation

C language was chosen because that is the language author is most comfortable with. It complicates writing interpreter compared to more higher-level languages while providing more power in low level control over resources.

That is to say, the process of writing language is not straightforward at all and required several sleepless nights spent of debugging memory corruption bugs.

Additional goal set at the start of the project is to create an online compiler/playground, similar to the ones languages like [Rust](https://play.rust-lang.org/) and [Go](https://go.dev/play/). For example, Go achieves this by having a web server receiving code from clients, compiling it and sending result. This, of course, is a fine solution for compiled language (although it can be done locally using something like [llvm bitcode interpreter](https://llvm.org/docs/CommandGuide/lli.html)). More simple solution it case of interpreted language was decided to be [WebAssembly](https://webassembly.org/). Long story short, we can compile our C code and call it from browser directly.

## Language design 

hololisp does not have discrete specification. It is a freestanding interpretation of ideas found in other lisp dialect. hololisp borrows ideas both from [common lisp](https://common-lisp.net/) and [scheme](https://www.scheme.com/tspl4/). More information about language features is located in the next chapters.

By its nature hololisp is extremely slow. It is slower than many popular lisp dialects, as well as other interpreted languages, like Python. Most of this time is spent on managing linked lists upon which the language is built on. For example, each function invocation loads and processes its arguments in linked list, spending a lot more time on a single call compared to using array for passing arguments.