;;
;; Game of life
;; hololisp version
;;

;; Define sizes here so we don't worry about passing them around
(defvar *width* 40)
(defvar *height* 40)

;; Modulo operator
(defun % (x y)
  (- x (* (/ x y) y)))

; ;; Function similar to python range used to create list of numbers from lo to hi
(defun range-lo (lo hi)
  (unless (<= hi lo)
    (cons lo (range-lo (+ lo 1) hi))))

; ;; Simplified range with only high value, starting from zero
(defun range (hi)
  (range-lo 0 hi))

; ;; Returns new matrix with width and *height* dimension

; ;; Prints board
(defun print-board (board)
  (if (not board)
    ()
    (progn (print (map (lambda (it) (if (= it 1) '@ '_)) (car board)))
           (print-board (cdr board)))))

(defun alive-at (board x y)
  (and (>= x 0)
       (< x *width*)
       (>= y 0)
       (< y *height*)
       (= (nth x (nth y board))
          1)))

(defun neighbours (board x y)
  (let ((at (lambda (x y) 
              (if (alive-at board x y) 1 0))))
      (+ (at (- x 1) (- y 1))
         (at (- x 1) y)
         (at (- x 1) (+ y 1))
         (at x (- y 1))
         (at x (+ y 1))
         (at (+ x 1) (- y 1))
         (at (+ x 1) y)
         (at (+ x 1) (+ y 1)))))

(defun should-be-alive (board x y)
  (let ((c (neighbours board x y)))
    (if (alive-at board x y)
      (or (= c 2) (= c 3))
      (= c 3))))

(defun next-state (board x y)
  (if (should-be-alive board x y)
    1
    0))

; ;; Run simulation step
; ;; Return new board after simulation step
(defun simulate (board)
  (map (lambda (y) 
         (map (lambda (x) 
                (next-state board x y))
              (range *width*)))
       (range *height*)))

(defun run (board)
  (while t 
    (clrscr)
    (print-board board)
    (setq board (simulate board))))

(defun create-row (width)
  (unless (<= width 0)
    (cons (% (rand) 2) (create-row (- width 1)))))

(defun create-board (width height)
  (unless (<= height 0)
    (cons (create-row width) (create-board width (- height 1)))))

(run (create-board *width* *height*))
