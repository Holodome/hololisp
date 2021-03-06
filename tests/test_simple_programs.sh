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

fib_source="(defun fib (n) \
(if (<= n 1) n \
(+ (fib (- n 1)) (fib (- n 2)))))"

run_test "fib 1" "1" "$fib_source (fib 1)"
run_test "fib 8" "21" "$fib_source (fib 8)"
run_test "fib 20" "6765" "$fib_source (fib 20)"

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
