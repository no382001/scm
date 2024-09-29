; included by core.scm

(define else #t) ; for cond

(define null? not)

(defun err? (x) (eq? x 'ERR))

(defun pair? (x) (not (err? (cdr x))))

(defun symbol? (x)
    (and
        x
        (not (err? x))
        (not (number? x))
        (not (pair? x))))

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

(defun test (name expr expected)
    (if (equal? expr expected)
        (begin
          (display 'test-passed-->)
          (display name))
        (begin
          (display 'test-failed-->)
          (display name)
          (newline)
          (display 'expected-->)
          (display expected)
          (newline)
          (display 'got-->)
          (display expr))))