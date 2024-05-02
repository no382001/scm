; common.scm

(define null? not)
(defun err? (x) (eq? x 'ERR))
(defun number? (x) (eq? (* 0 x) 0))
(defun pair? (x) (not (err? (cdr x))))
(defun symbol? (x)
    (and
        x
        (not (err? x))
        (not (number? x))
        (not (pair? x))))
(defun atom? (x)
    (or
        (not x)
        (symbol? x)))
(defun list? (x)
    (if (not x)
        #t
        (if (pair? x)
            (list? (cdr x))
            ())))
(defun equal? (x y)
    (or
        (eq? x y)
        (and
            (pair? x)
            (pair? y)
            (equal? (car x) (car y))
            (equal? (cdr x) (cdr y)))))
(defun negate (n) (- 0 n))
(defun > (x y) (< y x))
(defun <= (x y) (not (< y x)))
(defun >= (x y) (not (< x y)))
(defun = (x y) (eq? (- x y) 0))
(defun cadr (x) (car (cdr x)))
(defun caddr (x) (car (cdr (cdr x))))

;(define begin (lambda (x . args) (if args (begin . args) x)))
