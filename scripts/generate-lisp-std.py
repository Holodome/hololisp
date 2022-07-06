#!/usr/bin/env python3

source = """
(defun caar (x)             (car (car x)))
(defun cadr (x)             (car (cdr x)))
(defun cdar (x)             (cdr (car x)))
(defun cddr (x)             (cdr (cdr x)))
(defun caaar (x)       (car (car (car x))))
(defun caadr (x)       (car (car (cdr x))))
(defun cadar (x)       (car (cdr (car x))))
(defun caddr (x)       (car (cdr (cdr x))))
(defun cdaar (x)       (cdr (car (car x))))
(defun cdadr (x)       (cdr (car (cdr x))))
(defun cddar (x)       (cdr (cdr (car x))))
(defun cdddr (x)       (cdr (cdr (cdr x))))
(defun caaaar (x) (car (car (car (car x)))))
(defun caaadr (x) (car (car (car (cdr x)))))
(defun caadar (x) (car (car (cdr (car x)))))
(defun caaddr (x) (car (car (cdr (cdr x)))))
(defun cadaar (x) (car (cdr (car (car x)))))
(defun cadadr (x) (car (cdr (car (cdr x)))))
(defun caddar (x) (car (cdr (cdr (car x)))))
(defun cadddr (x) (car (cdr (cdr (cdr x)))))
(defun cdaaar (x) (cdr (car (car (car x)))))
(defun cdaadr (x) (cdr (car (car (cdr x)))))
(defun cdadar (x) (cdr (car (cdr (car x)))))
(defun cdaddr (x) (cdr (car (cdr (cdr x)))))
(defun cddaar (x) (cdr (cdr (car (car x)))))
(defun cddadr (x) (cdr (cdr (car (cdr x)))))
(defun cdddar (x) (cdr (cdr (cdr (car x)))))
(defun cddddr (x) (cdr (cdr (cdr (cdr x)))))

(defun rem (x y)
  (- x (* (/ x y) y)))
"""

for line in source.split("\n"):
    print(f"\"{line}\\n\"")
