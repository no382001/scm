; math.scm

(define abs
    (lambda (n)
        (if (< n 0)
            (- 0 n)
            n)))
(define frac (lambda (n) (- n (int n))))
(define truncate int)
(define floor
    (lambda (n)
        (int
            (if (< n 0)
                (- n 1)
                n))))
(define ceiling (lambda (n) (- 0 (floor (- 0 n)))))
(define round (lambda (n) (+ (floor n) 0.5)))
(define mod (lambda (n m) (- n (* m (int (/ n m))))))
(define gcd
    (lambda (n m)
        (if (eq? m 0)
            n
            (gcd m (mod n m)))))
(define lcm (lambda (n m) (/ (* n m) (gcd n m))))
(define even? (lambda (n) (eq? (mod n 2) 0)))
(define odd? (lambda (n) (eq? (mod n 2) 1)))

(define factorial (lambda (n)
  (if (< n 2)
      1
      (* n (factorial (- n 1))))))

(define power (lambda (base exp)
  (if (eq? exp 0)
      1
      (* base (power base (- exp 1))))))

; sine function using Maclaurin series approximation
(defun sin  (x)
  (+ x
    (- (/ (power x 3) (factorial 3)))
    (+ (/ (power x 5) (factorial 5)))
    (- (/ (power x 7) (factorial 7)))))

; cosine function using Maclaurin series approximation
(define cos (lambda (x)
  (+ 1
     (- (/ (power x 2) (factorial 2)))
     (+ (/ (power x 4) (factorial 4)))
     (- (/ (power x 6) (factorial 6))))))