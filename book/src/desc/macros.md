# Macros

As most lisp implementations, hololisp supports macros.

Macro, in hololisp terms, is a form that can be invoked during compile time producing hololisp code. Macros benefits are
best understood with an example.

## Why macros are necessary

Consider the case where we need to implement *and* operator, where it
should [short-circuit](https://en.wikipedia.org/wiki/Short-circuit_evaluation). That means if one of the terms evaluates
to false, others should not be evaluated.

First, let us try to implement it using a function.

```scheme
(define (&& expr . rest)
  (define (continue expr rest)
    (when expr
      (continue (car rest) (cdr rest))))
  (continue expr rest))

(print (&& 1 () 3)) ;; ()
```

In order to accept variable number of arguments this function needs to be recursive. By iterating all
arguments we check if any of them is nil and abort in that case. The code above behaves as expected, however it has one
downside. Consider the following invocation:

```scheme
(define x 0)
(print (&& (progn
             (set! x 1)
             x)
         ()
         (progn
           (set! x 2)
           x)
         (progn
           (set! x 3)
           x)))
; ()
(print x) ; 3
```

The value printed as result of '&&' is still calculated correctly, however, the program does not
short-circuit. When calling a function its arguments are evaluated,
producing the argument list `(1 2 3)` and changing the value of `x` to `3` as a side effect. In short-circuiting
implementation the program should print `1` because arguments past `()` are not evaluated.

The problem described above can be solved using macros.

## Hololisp macros

Macros in hololisp are similar to Common Lisp ones. A macro is a regular function that is invoked during compilation.

The correct implementation of '&&' described above should be:

```scheme
(defmacro (&& expr . rest)
  (if rest
    (list 'if expr (cons 'and rest))
    expr))
```

Macros evaluate to hololisp code. `(&& 1 2 3)` will produce the same code as `(if 1 (if 2 3))`.

## Difference with C macros

Programmers coming from C can be surprised when first introduced to lisp languages how frequently macros are used there.
There is distinct difference between C macros and lisp ones. C macros operate on token streams and are not aware of
language semantics because they actually come from the preprocessor, which is a whole different language, while lisp
macros are first-level language feature.

* [Why are preprocessor macros evil and what are the alternatives?](https://stackoverflow.com/questions/14041453/why-are-preprocessor-macros-evil-and-what-are-the-alternatives)
* [What makes Lisp macros so special?](https://stackoverflow.com/questions/267862/what-makes-lisp-macros-so-special)
