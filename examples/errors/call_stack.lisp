;; hololisp
;; this function demonstrates how error messages deep in call stack are displayed

(defun my-fun (x)
  (* (+ x 2)))

(defun my-other-fun(x)
  (/ (my-fun x) 10))

(print (my-other-fun 'abc))
