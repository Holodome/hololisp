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

    result=$("$EXECUTABLE" "$TMPFILE" 2> /dev/null | tail -1)
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
