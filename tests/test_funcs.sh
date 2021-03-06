#!/usr/bin/env bash

EXECUTABLE=./build/hololisp

TMPFILE=/tmp/hololisp.lisp

failed=0

panic () {
    echo -n -e '\e[1;31m[ERROR]\e[0m '
    echo "$1"
    failed=1
}

run_test () {
    echo -n "Testing $1 ... "

    echo "$3" > "$TMPFILE"
    error=$("$EXECUTABLE" < "$TMPFILE" 2>&1 > /dev/null)
    if [ -n "$error" ]; then
        echo FAILED
        panic "$error"
        return 
    fi

    result=$("$EXECUTABLE" < "$TMPFILE" 2> /dev/null | tail -1)
    if [ "$result" != "$2" ]; then
        echo FAILED
        panic "'$2' expected, but got '$result'"
        return 
    fi

    echo "ok"
}

run_test "nth nil" "()" "(nth 0 ())"
run_test "nth more than length nil" "()" "(nth 100 ())"
run_test "nth first" "0" "(nth 0 '(0 1 2 3))"
run_test "nth second" "1" "(nth 1 '(0 1 2 3))"
run_test "nth more than length" "()" "(nth 100 '(0 1 2 3))"

run_test "nthcdr nil" "()" "(nth 0 ())"
run_test "nthcdr more than length nil" "()" "(nthcdr 100 ())"
run_test "nthcdr first" "(0 1 2 3)" "(nthcdr 0 '(0 1 2 3))"
run_test "nthcdr second" "(1 2 3)" "(nthcdr 1 '(0 1 2 3))"
run_test "nthcdr more than length" "()" "(nthcdr 100 '(0 1 2 3))"

run_test "setcar" "(0 1)" "(setcar '(() 1) 0)"
run_test "setcdr" "(1 2)" "(setcdr '(1) '(2))"

run_test "any true" "t" "(any (lambda (x) x) '(() 1 () '(1 2 3)))"
run_test "any nil" "()" "(any (lambda (x) x) '(() () () ((lambda () ()))))"

run_test "map" "(1 2 3 4)" "(map (lambda (x) (+ x 1)) '(0 1 2 3))"
run_test "when true" "t" "(when t t)"
run_test "when false" "()" "(when () t)"
run_test "unless true" "()" "(unless t t)"
run_test "unless false" "t" "(unless () t)"

run_test "or true" "t" "(or () () 1)"
run_test "or false" "()" "(or () () ())"
run_test "and true" "t" "(and 1 2 3)"
run_test "and false" "()" "(and () () ())"

run_test "listp" "t" "(listp '(1 2 3))"
run_test "listp" "t" "(listp ())"
run_test "listp" "()" "(listp 1)"
run_test "listp" "()" "(listp 'abc)"

run_test "null" "()" "(null '(1 2 3))"
run_test "null" "t" "(null ())"
run_test "null" "()" "(null 1)"
run_test "null" "()" "(null 'abc)"

run_test "numberp" "()" "(numberp '(1 2 3))"
run_test "numberp" "()" "(numberp ())"
run_test "numberp" "t" "(numberp 1)"
run_test "numberp" "()" "(numberp 'abc)"

run_test "plusp" "t" "(plusp 100)"
run_test "plusp" "()" "(plusp 0)"
run_test "plusp" "()" "(plusp -100)"

run_test "zerop" "()" "(zerop 100)"
run_test "zerop" "t" "(zerop 0)"
run_test "zerop" "()" "(zerop -100)"

run_test "minusp" "()" "(minusp 100)"
run_test "minusp" "()" "(minusp 0)"
run_test "minusp" "t" "(minusp -100)"

run_test "append" "(a b c d)" "(append '(a b) '(c d))"
run_test "append" "(1 (2 (3)) i (j) k)" "(append '(1 (2 (3))) '(i (j) k))"
run_test "append" "(foo . bar)" "(append '(foo) 'bar)"

run_test "reverse" "()" "(reverse ())"
run_test "reverse" "(1)" "(reverse '(1))"
run_test "reverse" "(4 3 2 1)" "(reverse '(1 2 3 4))"

run_test "min" "1" "(min 1 2 3 4)"
run_test "max" "4" "(max 1 2 3 4)"

run_test "abs" "1" "(abs -1)"
run_test "abs" "1" "(abs 1)"

run_test "progn" "()" "(progn)"
run_test "progn" "t" "(progn t)"
run_test "progn" "(1 2 3)" "(progn 'a (list 1 2 3))"

exit $failed
