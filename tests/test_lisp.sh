#!/usr/bin/env bash

EXECUTABLE=./build/hololisp

failed=0

panic () {
    echo -n -e '\e[1;31m[ERROR]\e[0m '
    echo "$1"
    failed=1
}

run_test () {
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

run_test comment 5 "
    ; 2
    5 ; 3"

run_test integer 1 1
run_test integer -1 -1

run_test nil "()" "()"
run_test "true" "t" "t"

run_test "+" 3 "(+ 1 2)"
run_test "+" -2 "(+ 1 -3)"

run_test "=" "t" "(= 1 1)"
run_test "=" "()" "(= 1 -1)"
run_test "=" "t" "(= 1 1 1 1)"
run_test "=" "()" "(= 1 1 1 -1)"

run_test "/=" "()" "(/= 1 1)"
run_test "/=" "t" "(/= 1 -1)"
run_test "/=" "t" "(/= 1 2 3 4)"
run_test "/=" "()" "(/= 1 2 3 1)"

run_test "<" "t" "(< 1 2)"
run_test "<" "()" "(< 1 -2)"
run_test "<" "t" "(< 1 2 3 4)"
run_test "<" "()" "(< 1 3 5 5)"

run_test "<=" "t" "(<= 1 1)"
run_test "<=" "()" "(<= 1 -2)"
run_test "<=" "t" "(<= 1 2 2 4)"
run_test "<=" "()" "(<= 1 1 1 -1)"

run_test ">" "t" "(> 2 1)"
run_test ">" "()" "(> -2 1)"
run_test ">" "t" "(> 4 3 2 1)"
run_test ">" "()" "(> -1 3 5 1)"

run_test ">=" "t" "(>= 1 1)"
run_test ">=" "()" "(>= -2 1)"
run_test ">=" "t" "(>= 4 2 2 1)"
run_test ">=" "()" "(>= -1 1 1 1)"

run_test "'" "1" "'1"
run_test "'" "abc" "'abc"
run_test "'" "(a b c)" "'(a b c)"

run_test "+ '" 10 "(+ 1 2 . (3 4))"
run_test ". '" "(1 . 2)" "'(1 . 2)"

run_test "car" "()" "(car ())"
run_test "cdr" "2" "(cdr '(1 . 2))"
run_test "cdr" "(2)" "(cdr '(1 2))" 
run_test "cadr" "2" "(cadr '(1 2))" 
run_test "car" "a" "(car '(a b c))" 
run_test "cdr" "(b c)" "(cdr '(a b c))"

run_test "cadr" "2" "(cadr '(1 2 3 4))"
run_test "caddr" "3" "(caddr '(1 2 3 4))"
run_test "cadddr" "4" "(cadddr '(1 2 3 4))"

run_test "if" "1" "(if t 1 2)"
run_test "if" "2" "(if () 1 2)"
run_test "if" "less" "(if (> 3 4) 'more 'less)"
run_test "if" "more" "(if (> 4 3) 'more 'less)"
run_test "if" "123" "(if t 123)"
run_test "if" "()" "(if () 123)"

run_test "if" "a" "(if 1 'a)"
run_test "if" "()" "(if () 'a)"
run_test "if" "a" "(if 1 'a 'b)"
run_test "if" "a" "(if 0 'a 'b)"
run_test "if" "a" "(if 'x 'a 'b)"
run_test "if" "b" "(if () 'a 'b)"

run_test "cons" "(a a b)" "(cons 'a '(a b))"
run_test "cons" "((a b) . a)" "(cons '(a b) 'a)"

run_test "list" "(1)" "(list 1)"
run_test "list" "()" "(list)"
run_test "list" "(1 2 3 4)" "(list (+ 0 1) (* 2 1) (/ 9 3) (- 0 -4))"

run_test "setcar" "(0 1)" "(setcar '(() 1) 0)"
run_test "setcdr" "(1 2)" "(setcdr '(1) '(2))"

run_test "defun" "double" "(defun double (x) (+ x x))"
run_test "defun & call" "12" "(defun double (x) (+ x x)) (double 6)"

run_test "let" "t" "(let ((x t)) x)"
run_test "let" "2" "(let ((x 1)) (+ x 1))"
run_test "let" "3" "(let ((c 1)) (let ((c 2) (a (+ c 1))) a))"

run_test "progn" "()" "(progn)"
run_test "progn" "t" "(progn t)"
run_test "progn" "(1 2 3)" "(progn 'a (list 1 2 3))"

fib_source="(defun fib (n) \
(if (<= n 1) n \
(+ (fib (- n 1)) (fib (- n 2)))))"

run_test "fib 1" "1" "$fib_source (fib 1)"
run_test "fib 8" "21" "$fib_source (fib 8)"
run_test "fib 20" "6765" "$fib_source (fib 20)"

run_test "lambda" "t" "((lambda () t))"
run_test "lambda" "9" "((lambda (x) (+ x x x)) 3)"
run_test "lambda" "2" "(defvar x 1)
((lambda (x) x) 2)"

run_test "closure" "3" "(defun call (f) ((lambda (var) (f)) 5))
  ((lambda (var) (call (lambda () var))) 3)"

fizzbuzz_source="(defun fizzbuzz (n) \
(let ((is-mult-p (lambda (mult) (= (rem n mult) 0)))) \
(let ((mult-3 (is-mult-p 3)) \
(mult-5 (is-mult-p 5))) \
(if (and mult-3 mult-5) \
'fizzbuzz \
(if mult-3 'fizz \
(if mult-5 'buzz n))))))"

run_test "fizzbuzz 3" "fizz" "$fizzbuzz_source (fizzbuzz 3)"
run_test "fizzbuzz 5" "buzz" "$fizzbuzz_source (fizzbuzz 5)"
run_test "fizzbuzz 9" "fizz" "$fizzbuzz_source (fizzbuzz 9)"
run_test "fizzbuzz 25" "buzz" "$fizzbuzz_source (fizzbuzz 25)"
run_test "fizzbuzz 15" "fizzbuzz" "$fizzbuzz_source (fizzbuzz 15)"
run_test "fizzbuzz 17" "17" "$fizzbuzz_source (fizzbuzz 17)"

exit $failed