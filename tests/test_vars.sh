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

run_test "let" "t" "(let ((x t)) x)"
run_test "let" "2" "(let ((x 1)) (+ x 1))"
run_test "let" "2" "(let ((c 1)) (let ((c 2) (a (+ c 1))) a))"

run_test "while with defvar and defq" "45" "(defvar i 0) \
  (defvar sum 0) \
  (while (< i 10) \
    (setq sum (+ sum i)) \
    (setq i (+ i 1))) \
  sum"

exit $failed
