# Macros

As most lisp implementations, hololisp supports macros.

Macro, in hololisp terms, is a form that can be invoked during compile time, producing code
that can not be acquired in other way. Macros benefits are best understood with an example.

## Example why macros are necessary

Consider the case where we need to implement *and* operator, where it should [short-circuit](https://en.wikipedia.org/wiki/Short-circuit_evaluation). That means if one of the terms evaluates to true, others should not be evaluated. 

First, let us see how this can be attempted to be implemented using a function.

```scheme
(define (&& expr . rest)
  (define (continue expr rest)
    (when expr
      (continue (car rest) (cdr rest))))
  (continue expr rest))

(print (&& () 2 3)) ;; 2
```

> Currently, it is not possible in language to use existing list as argument list. This forbids writing code above using single recursive function, and we have to use local recursive one instead.

First note that in order to accept variable number of arguments this function needs to be recursive. By iterating all arguments we decide which one is first non-nil and return its value as result. The code above behaves as expected, however it has one downside. Consider the following invocation:

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

The value printed as result of 'and' is still calculated correctly, however, the program behaviour does not short-circuit. When calling a function all of its arguments are evaluated, 
thus producing the argument list `(1 2 3)` and changing the value of x to 3. In short circtuting implementation the program should only evaluate following arguments only if the current evaluates to false, thus we expect `(print x)` print 1.

The problem described above can be solved using macros.

## Hololisp macros

Macros in hololisp similar to Common Lisp ones. A macro is a regular function that is invoked during compilation. Because of that fact, macro can choose which arguments need or not need to be evaluated at any given time.

The correct implementation of 'and' described above should be:

```scheme
(defmacro (&& expr . rest)
  (if rest
    (list 'if expr (cons 'and rest))
    expr))
```

Macros do not produce result when evaluated. Instead, they produce hololisp code. For example, macro can be used to generate any valid hololisp code. For example `(&& 1 2 3)` will produce  the same code as `(if 1 (if 2 3))`

Because 'if' special form is guaranteed to evaluate only one of its arms during invocation, we are able to use this in order to get short-circuiting behaviour any place we want. For example, 'cond' macro found in most other lisps evaluates to similar code making use of 'if's.

## Aren't macros bad?

Programmers coming from C can be surprised when first introduced to lisp languages how frequently macros are used there. There is distinct difference between C macros and lisp ones. C macros operate on token streams and are not aware of language semantics because they actually come from the preprocessor, which is a whole different language, while lisp macros are first-level language feature.  

* [Why are preprocessor macros evil and what are the alternatives?](https://stackoverflow.com/questions/14041453/why-are-preprocessor-macros-evil-and-what-are-the-alternatives)
* [What makes Lisp macros so special?](https://stackoverflow.com/questions/267862/what-makes-lisp-macros-so-special)
