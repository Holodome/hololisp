#!/usr/bin/env bash

EXECUTABLE=./build/hololisp-format
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

    result=$("$EXECUTABLE" < "$TMPFILE" 2> /dev/null1)
    if [ "$result" != "$2" ]; then
        echo FAILED
        panic "'$2' expected, but got '$result'"
        return 
    fi

    echo "ok"
}

test_src="(defun f (x) (when (< (g x) 3) (h x 2)))"
test_out=$(cat <<- EOF 
(defun f (x)
    (when (< (g x) 3)
       (h x 2)))
EOF
)
run_test "closing parens" "$test_out" "$test_src"
# Ok this is the same test but whatever
run_test "ident is two" "$test_out" "$test_src"

test_src="(if (< (g x) 2)     ; is it sufficiently small?
              (top-level x)   ; if so, abandon everything
              (h y))          ; otherwise try again"
test_out=$(cat <<- EOF 
(if (< (g x) 2)    ; is it sufficiently small?
    (top-level x)  ; if so, abandon everything
    (h y))         ; otherwise try again
EOF
)

# Two spaces are placed after the line
# TODO: Think how this can be done better (line up groups?)
run_test "single column comment" "$test_out" "$test_src"

test_src="(if (< (g x) 2)     
;; reinitialize and abandon everything
              (top-level x)  
              (h y))"
test_out=$(cat <<- EOF 
(if (< (g x) 2)
    ;; reinitialize and abandon everything
    (top-level x) 
    (h y))
EOF
)
run_test "double column comment" "$test_out" "$test_src"

test_src="(abc (< (g x) 2) ;; reinitialize and abandon everything
            (top-level x)  
            (h y))"

run_test "double column comment" "$test_out" "$test_src"

test_src="(if (= (f x) 4) (top-level x) (g x))"
test_out=$(cat <<- EOF
(if (< (g x) 2) 
    (top-level x) 
    (h y))
EOF
)
run_test "if special form" "$test_out" "$test_src"

test_src="(when (= (f x) 4)
    (setf *level-number* 0)
    (top-level x))"
test_out=$(cat <<- EOF
(when (= (f x) 4)
    (setf *level-number* 0)
    (top-level x))
EOF
)

run_test "when special form" "$test_out" "$test_src"

test_src="(unless *do-not-reinitialize*
  (reinitialize-global-information x)
  (reinitialize-local-information))"
test_out=$(cat <<- EOF
(unless *do-not-reinitialize*
  (reinitialize-global-information x)
  (reinitialize-local-information))
EOF
)

run_test "unless special form" "$test_out" "$test_src"

test_src="(when (= (f x) 4)
    (setf *level-number* 0)
    (unless *do-not-reinitialize*
      (reinitialize-global-information x)
      (reinitialize-local-information))
    (top-level x))"
test_out=$(cat <<- EOF
(when (= (f x) 4)
  (setf *level-number* 0)
  (unless *do-not-reinitialize*
    (reinitialize-global-information x)
    (reinitialize-local-information))
  (top-level x))
EOF
)

run_test "when and unless special forms" "$test_out" "$test_src"

test_src="(defun my-fun (x y z) (* x y z))"
test_out=$(cat <<- EOF
(defun my-fun (x y z)
  (* x y z))
EOF
)
run_test "defun special form" "$test_out" "$test_src"

test_src="(let ((symbols (mapcar #'compute-symbol l))
         (spaces (mapcar #'compute-space symbols (cdr symbols))))
    (when (verify-spacing symbols spaces)
      (make-spacing permanent spaces)))"
test_out=$(cat <<- EOF
(let ((symbols (mapcar #'compute-symbol l))
      (spaces (mapcar #'compute-space symbols (cdr symbols))))
  (when (verify-spacing symbols spaces)
    (make-spacing permanent spaces)))
EOF
)
run_test "let special form" "$test_out" "$test_src"

test_src="(my-fun 1 2 3)"
test_out=$(cat <<- EOF
(my-fun 1
        2 
        3)
EOF
)
run_test "function arguments" "$test_out" "$test_src"

test_src="((1) (2) (3))"
test_out=$(cat <<- EOF
((1)
 (2)
 (3))
EOF
)
run_test "list of lists" "$test_out" "$test_src"

test_src="(defun small-prime-number-p (n)
  (cond ((or (< n 2))
         nil)
        ((= n 2)   ; parenthetical remark here
         t)        ; continuation of the remark
        ((divisorp 2 n)
         nil)  ; different remark
        ;; Comment that applies to a section of code.
        (t
         (loop for i from 3 upto (sqrt n) by 2
               never (divisorp i n)))))"
test_out=$(cat <<- EOF
(defun small-prime-number-p (n)
  (cond ((or (< n 2))
         nil)
        ((= n 2)  ; parenthetical remark here
         t)       ; continuation of the remark
        ((divisorp 2 n) nil)  ; different remark
        ;; Comment that applies to a section of code.
        (t
         (loop for i from 3 upto (sqrt n) by 2
           never (divisorp i n)))))
EOF
)
run_test "complex" "$test_out" "$test_src"

exit $failed
