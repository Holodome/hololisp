#!/usr/bin/env bash

EXECUTABLE=./build/hololisp

TMPFILE=/tmp/hololisp.lisp

panic () {
    echo -n -e '\e[1;31m[ERROR]\e[0m '
    echo "$1"
    # exit 1
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

run_test "cons" "(a a b)" "(cons 'a '(a b))"
run_test "cons" "((a b) . a)" "(cons '(a b) 'a)"

run_test "list" "(1)" "(list 1)"
run_test "list" "()" "(list)"
run_test "list" "(1 2 3 4)" "(list (+ 0 1) (* 2 1) (/ 9 3) (- 0 -4))"
