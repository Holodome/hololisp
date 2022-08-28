#!/usr/bin/env bash

EXECUTABLE=./build/hololisp

failed=0

panic () {
    echo -n -e '\e[1;31m[ERROR]\e[0m '
    echo "$1"
    failed=1
}

pos_test () {
    echo -n "Testing $1 ... "

    error=$("$EXECUTABLE" -e "$3" 2>&1 > /dev/null)
    if [ -n "$error" ]; then
        echo FAILED
        panic "$error"
        return 
    fi

    result=$("$EXECUTABLE" -e "$3" 2> /dev/null | tail -1)
    if [ "$result" != "$2" ]; then
        echo FAILED
        panic "'$2' expected, but got '$result'"
        return 
    fi

    echo "ok"
}

neg_test () {
    echo -n "Testing $1 ... "
     error=$("$EXECUTABLE" -e "$2" 2>&1 > /dev/null)
    if [ -z "$error" ]; then
        echo FAILED
        panic "$1"
        return
    fi

    echo "ok"
}

pos_test comment 5 "
    ; 2
    5 ; 3"

pos_test integer 1 1
pos_test integer -1 -1

pos_test nil "()" "()"
pos_test "true" "t" "t"

pos_test "+" 3 "(+ 1 2)"
pos_test "+" -2 "(+ 1 -3)"

pos_test "=" "t" "(= 1 1)"
pos_test "=" "()" "(= 1 -1)"
pos_test "=" "t" "(= 1 1 1 1)"
pos_test "=" "()" "(= 1 1 1 -1)"

pos_test "/=" "()" "(/= 1 1)"
pos_test "/=" "t" "(/= 1 -1)"
pos_test "/=" "t" "(/= 1 2 3 4)"
pos_test "/=" "()" "(/= 1 2 3 1)"

pos_test "<" "t" "(< 1 2)"
pos_test "<" "()" "(< 1 -2)"
pos_test "<" "t" "(< 1 2 3 4)"
pos_test "<" "()" "(< 1 3 5 5)"

pos_test "<=" "t" "(<= 1 1)"
pos_test "<=" "()" "(<= 1 -2)"
pos_test "<=" "t" "(<= 1 2 2 4)"
pos_test "<=" "()" "(<= 1 1 1 -1)"

pos_test ">" "t" "(> 2 1)"
pos_test ">" "()" "(> -2 1)"
pos_test ">" "t" "(> 4 3 2 1)"
pos_test ">" "()" "(> -1 3 5 1)"

pos_test ">=" "t" "(>= 1 1)"
pos_test ">=" "()" "(>= -2 1)"
pos_test ">=" "t" "(>= 4 2 2 1)"
pos_test ">=" "()" "(>= -1 1 1 1)"

pos_test "'" "1" "'1"
pos_test "'" "abc" "'abc"
pos_test "'" "(a b c)" "'(a b c)"

pos_test "+ '" 10 "(+ 1 2 . (3 4))"
pos_test ". '" "(1 . 2)" "'(1 . 2)"

neg_test "car args" "(car)"
neg_test "car args" "(car () ())"
neg_test "cdr args" "(cdr)"
neg_test "cdr args" "(cdr () ())"
pos_test "car" "()" "(car ())"
pos_test "cdr" "2" "(cdr '(1 . 2))"
pos_test "cdr" "(2)" "(cdr '(1 2))" 
pos_test "cadr" "2" "(cadr '(1 2))" 
pos_test "car" "a" "(car '(a b c))" 
pos_test "cdr" "(b c)" "(cdr '(a b c))"

pos_test "cadr" "2" "(cadr '(1 2 3 4))"
pos_test "caddr" "3" "(caddr '(1 2 3 4))"
pos_test "cadddr" "4" "(cadddr '(1 2 3 4))"

neg_test "if args" "(if)"
neg_test "if args" "(if t)"
pos_test "if" "1" "(if t 1 2)"
pos_test "if" "2" "(if () 1 2)"
pos_test "if" "less" "(if (> 3 4) 'more 'less)"
pos_test "if" "more" "(if (> 4 3) 'more 'less)"
pos_test "if" "123" "(if t 123)"
pos_test "if" "()" "(if () 123)"
pos_test "if progn" "1" "(if () t t 1)"

pos_test "if" "a" "(if 1 'a)"
pos_test "if" "()" "(if () 'a)"
pos_test "if" "a" "(if 1 'a 'b)"
pos_test "if" "a" "(if 0 'a 'b)"
pos_test "if" "a" "(if 'x 'a 'b)"
pos_test "if" "b" "(if () 'a 'b)"

neg_test "cons args" "(cons)"
neg_test "cons args" "(cons 1)"
neg_test "cons args" "(cons 1 2 3)"
pos_test "cons" "(a a b)" "(cons 'a '(a b))"
pos_test "cons" "((a b) . a)" "(cons '(a b) 'a)"

pos_test "list" "(1)" "(list 1)"
pos_test "list" "()" "(list)"
pos_test "list" "(1 2 3 4)" "(list (+ 0 1) (* 2 1) (/ 9 3) (- 0 -4))"

neg_test "setcar! args" "(setcar!)"
neg_test "setcar! args" "(setcar! '(1))"
pos_test "setcar!" "(0 1)" "(setcar! '(() 1) 0)"
neg_test "setcdr! args" "(setcdr!)"
neg_test "setcdr! args" "(setcdr! '(1))"
pos_test "setcdr!" "(1 2)" "(setcdr! '(1) '(2))"

neg_test "define args" "(define)"
neg_test "define name" "(define (1) ())"
neg_test "define args" "(define (1))"
pos_test "define" "double" "(define (double x) (+ x x))"
pos_test "define & call" "12" "(define (double x) (+ x x)) (double 6)"

neg_test "let args" "(let)"
pos_test "let" "t" "(let ((x t)) x)"
pos_test "let" "2" "(let ((x 1)) (+ x 1))"
pos_test "let" "3" "(let ((c 1)) (let ((c 2) (a (+ c 1))) a))"
pos_test "let" "2" "(let ((x (+ 0 1))) (+ x 1))"

pos_test "progn" "()" "(progn)"
pos_test "progn" "t" "(progn t)"
pos_test "progn" "(1 2 3)" "(progn 'a (list 1 2 3))"

fib_source="(define (fib n) \
(if (<= n 1) n \
(+ (fib (- n 1)) (fib (- n 2)))))"

pos_test "fib 1" "1" "$fib_source (fib 1)"
pos_test "fib 8" "21" "$fib_source (fib 8)"
pos_test "fib 20" "6765" "$fib_source (fib 20)"

neg_test "lambda args" "(lambda)"
neg_test "lambda args" "(lambda ())"
neg_test "lambda wrong param list" "(lambda 1 ())"
neg_test "lambda wrong param list" "(lambda (1) ())"
pos_test "lambda" "t" "((lambda () t))"
pos_test "lambda" "9" "((lambda (x) (+ x x x)) 3)"
pos_test "lambda" "2" "(define x 1)
((lambda (x) x) 2)"

pos_test "closure" "3" "(define (call f) ((lambda (var) (f)) 5))
  ((lambda (var) (call (lambda () var))) 3)"
pos_test "closure" "3" "(define x 2) (define f (lambda () x)) (set! x 3) (f)"

fizzbuzz_source="(define (fizzbuzz n) \
(let ((is-mult-p (lambda (mult) (= (rem n mult) 0)))) \
(let ((mult-3 (is-mult-p 3)) \
(mult-5 (is-mult-p 5))) \
(if (and mult-3 mult-5) \
'fizzbuzz \
(if mult-3 'fizz \
(if mult-5 'buzz n))))))"

pos_test "fizzbuzz 3" "fizz" "$fizzbuzz_source (fizzbuzz 3)"
pos_test "fizzbuzz 5" "buzz" "$fizzbuzz_source (fizzbuzz 5)"
pos_test "fizzbuzz 9" "fizz" "$fizzbuzz_source (fizzbuzz 9)"
pos_test "fizzbuzz 25" "buzz" "$fizzbuzz_source (fizzbuzz 25)"
pos_test "fizzbuzz 15" "fizzbuzz" "$fizzbuzz_source (fizzbuzz 15)"
pos_test "fizzbuzz 17" "17" "$fizzbuzz_source (fizzbuzz 17)"

neg_test "min args" "(min)"
neg_test "min type" "(min ())"
pos_test "min" "1" "(min 1 2 3 4)"
neg_test "max args" "(max)"
neg_test "max type" "(min ())"
pos_test "max" "4" "(max 1 2 3 4)"

neg_test "list? args" "(list?)"
neg_test "list? args" "(list? () 1)"
pos_test "list?" "t" "(list? '(1 2 3))"
pos_test "list?" "t" "(list? ())"
pos_test "list?" "()" "(list? 1)"
pos_test "list?" "()" "(list? 'abc)"

neg_test "null? args" "(null?)"
neg_test "null? args" "(null? () ())"
pos_test "null?" "()" "(null? '(1 2 3))"
pos_test "null?" "t" "(null? ())"
pos_test "null?" "()" "(null? 1)"
pos_test "null?" "()" "(null? 'abc)"

neg_test "number? args" "(number?)"
neg_test "number? args" "(number? () ())"
pos_test "number?" "()" "(number? '(1 2 3))"
pos_test "number?" "()" "(number? ())"
pos_test "number?" "t" "(number? 1)"
pos_test "number?" "()" "(number? 'abc)"

neg_test "positive? args" "(positive?)"
neg_test "positive? args" "(positive? () ())"
neg_test "positive? type" "(positive? ())"
pos_test "positive?" "t" "(positive? 100)"
pos_test "positive?" "()" "(positive? 0)"
pos_test "positive?" "()" "(positive? -100)"

neg_test "zero? args" "(zero?)"
neg_test "zero? args" "(zero? 1 2)"
neg_test "zero? type" "(zero? ())"
pos_test "zero?" "()" "(zero? 100)"
pos_test "zero?" "t" "(zero? 0)"
pos_test "zero?" "()" "(zero? -100)"

neg_test "negative? args" "(negative?)"
neg_test "negative? args" "(negative? 1 2)"
neg_test "negative? type" "(negative? ())"
pos_test "negative?" "()" "(negative? 100)"
pos_test "negative?" "()" "(negative? 0)"
pos_test "negative?" "t" "(negative? -100)"

neg_test "abs args" "(abs)"
neg_test "abs args" "(abs 1 2)"
neg_test "abs type" "(abs ())"
pos_test "abs" "1" "(abs -1)"
pos_test "abs" "1" "(abs 1)"

pos_test "append" "()" "(append)"
pos_test "append" "(a b)" "(append '(a b))"
pos_test "append" "(a b c d)" "(append '(a b) '(c d) ())"
pos_test "append" "(a b c d)" "(append '(a b) '(c d))"
pos_test "append" "(1 (2 (3)) i (j) k)" "(append '(1 (2 (3))) '(i (j) k))"
pos_test "append" "(foo . bar)" "(append '(foo) 'bar)"

neg_test "reverse args" "(reverse)"
neg_test "reverse args" "(reverse () ())"
pos_test "reverse" "()" "(reverse ())"
pos_test "reverse" "(1)" "(reverse '(1))"
pos_test "reverse" "(4 3 2 1)" "(reverse '(1 2 3 4))"

pos_test "when true" "t" "(when t t)"
pos_test "when false" "()" "(when () t)"
pos_test "unless true" "()" "(unless t t)"
pos_test "unless false" "t" "(unless () t)"

pos_test "or true" "1" "(or () () 1)"
pos_test "or false" "()" "(or () () ())"
pos_test "or empty" "()" "(or)"
pos_test "and true" "3" "(and 1 2 3)"
pos_test "and false" "()" "(and () () ())"
pos_test "and empty" "t" "(and)"

pos_test "not true" "()" "(not t)"
pos_test "not nil" "t" "(not ())"
pos_test "not eval" "()" "(not (+ 1 2))"

pos_test "set!" "321" "(define x 123)
(set! x 321)
x"
pos_test "set!" "(1 100 3)" "
(define x '(1 2 3))
(define c (cdr x))
(set! (car (cdr x)) 100)
x"
pos_test "set!" "(1 100 3)" "
(define x '(1 2 3))
(define c (cdr x))
(set! (cadr x) 100)
x"

neg_test "nthcdr args" "(nthcdr)"
neg_test "nthcdr args" "(nthcdr 0)"
neg_test "nthcdr args" "(nthcdr 0 () ())"
neg_test "nthcdr negative" "(nthcdr -1 ())"
neg_test "nthcdr non-number" "(nthcdr () ())"
pos_test "nthcdr nil" "()" "(nthcdr 0 ())"
pos_test "nthcdr more than length nil" "()" "(nthcdr 100 ())"
pos_test "nthcdr first" "(0 1 2 3)" "(nthcdr 0 '(0 1 2 3))"
pos_test "nthcdr second" "(1 2 3)" "(nthcdr 1 '(0 1 2 3))"
pos_test "nthcdr more than length" "()" "(nthcdr 100 '(0 1 2 3))"

neg_test "nth args" "(nth)"
neg_test "nth args" "(nth 0)"
neg_test "nth args" "(nth 0 () ())"
neg_test "nth negative" "(nth -1 ())"
neg_test "nth non-number" "(nth () ())"
pos_test "nth nil" "()" "(nth 0 ())"
pos_test "nth more than length nil" "()" "(nth 100 ())"
pos_test "nth first" "0" "(nth 0 '(0 1 2 3))"
pos_test "nth second" "1" "(nth 1 '(0 1 2 3))"
pos_test "nth more than length" "()" "(nth 100 '(0 1 2 3))"

pos_test "set! nth" "(100 2 3)" "(define x '(1 2 3)) (set! (nth 0 x) 100) x"

pos_test "macro" "19" "(defmacro mac1 (a b) (list '+ a (list '* b 3))) (mac1 4 5)"
pos_test "macro" "7" "(defmacro seven () 7) ((lambda () (seven)))"
pos_test "macro" "42" "(defmacro if-zero (x then) (list 'if (list '= x 0) then))
  (if-zero 0 42)"

pos_test "macroexpand" '(if (= x 0) (print x))' "
  (defmacro if-zero (x then) (list 'if (list '= x 0) then))
  (macroexpand (if-zero x (print x)))"

pos_test "restargs" "(3 5 7)" "(define (f x . y) (cons x y)) (f 3 5 7)"
pos_test "restargs" "(3)" "(define (f x . y) (cons x y)) (f 3)"
pos_test "restargs" "empty" "(define (f . rest) (if (null? rest) 'empty 'not-empty)) (f)"
pos_test "restargs" "not-empty" "(define (f . rest) (if (null? rest) 'empty 'not-empty)) (f 1 2 3)"

neg_test "random args" "(random ())"
neg_test "random args" "(random 1 2 3)"
pos_test "random" "()" "(= (random) (random) (random) (random) (random) (random) (random) (random) (random) (random))"
pos_test "random high" "t" "(define (test) (< (random 1024) 1024)) (and (test) (test) (test) (test) (test))"
pos_test "random high low" "t" "(define (test) (let ((x (random 1024 2048))) (and (< x 2048) (>= x 1024)))) (and (test) (test) (test) (test) (test))"

pos_test "scopes" "10" "(define (f) y) (define y 10) (f)"
neg_test "scopes" "(define (f x) (g)) (define (g) x) (f 10)"
pos_test "closure scopes" "10" "(define (f) (define fn (lambda () x)) (define x 10) fn) ((f))"

# pos_test "restargs macro" "(if 1 (if 2 3))" "(defmacro && (expr . rest)
                                # (if rest
                                 #   (list 'if expr (&& rest))
                                    #expr))
# (macroexpand (&& 1 2 3))"

exit $failed
