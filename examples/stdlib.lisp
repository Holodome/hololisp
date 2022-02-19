;; Standart list
;; lambda
;; cons 
;; car
;; cdr

(defmacro let (var val . body)
  (cons (cons 'lambda (cons (list var) body))
        (list val)))

(defmacro progn (expr . rest)
  (list (cons 'lambda () (cons expr rest))))

(defun list (a . b)
  (cons a b))

(defmacro when (expr . body)
  (cons 'if (cons expr (list (cons 'progn body)))))

(defmacro unless (expr . body)
  (cons 'if (cons expr (cons () body))))

(defun any (lst)
  (when lst
    (or (car lst)
        (any (cdr lst)))))

(defun map (lst fn)
  (when lst
    (cons (fn (car lst))
          (map (cdr lst) fn))))

(defun filter (lst fn)
  (when lst
    (let cur (car lst) 
      (if (fn cur)
        (cons cur (filter (cdr lst) fn))
        (filter (cdr lst) fn)))))

(defun reduce (lst fn)
  (if (lst cdr)
    (fn (lst car) (reduce (lst cdr) fn))
    (lst car)))

(defun nth (lst n)
  (if (= n 0)
    (car lst)
    (nth (cdr lst) (- n 1))))

(defun foreach (lst fn)
  (or (not lst)))


(defun %iota (a b)
  (unless (<= a b)
    (cons a (%iota (+ a 1) b))))

(defun iota (n)
  (%iota 0 n))

;; Standard conditional
;; if 

(defun not (a)
  (if a () t))
;; (and a b ...) => (if a (and b ...))
;; (and a) => a
(defmacro and (expr . rest)
  (if rest
    (list 'if expr (cons 'and rest))
    expr))

;; (or a b ...) => (if (not a) (or b ...) T)
;; (or a) => a 
(defmacro or (expr . rest)
  (if rest
    (list 'if (not expr) (cons 'or rest) t) 
    expr))

;; (or a b ...) => if (

;; Standard arithmetic operators: 
;; +
;; - 
;; *
;; /
;; <
;; =
;; <<
;; >>
;; &
;; |
;; ^

(defun <= (a b)
  (or (< a b)
      (= a b)))

(defun > (a b)
  (not (<= a b)))

(defun >= (a b)
  (not (< a b)))

(defun max (a b)
  (if (> a b) a b))

(defun min (a b)
  (if (< a b) a b))

(defun % (a b)
  (- a (* b (/ a b))))
