; math.scm

(defun abs (n)
        (if (< n 0)
            (- 0 n)
            n))
(defun frac (n) (- n (int n)))
(defun truncate int)
(defun floor (n)
        (int
            (if (< n 0)
                (- n 1)
                n)))
(defun ceiling (n) (- 0 (floor (- 0 n))))
(defun round (n) (+ (floor n) 0.5))
(defun mod (n m) (- n (* m (int (/ n m)))))
(defun gcd (n m)
        (if (eq? m 0)
            n
            (gcd m (mod n m))))
(defun lcm (n m) (/ (* n m) (gcd n m)))
(defun even? (n) (eq? (mod n 2) 0))
(defun odd? (n) (eq? (mod n 2) 1))

(defun factorial (n)
  (if (< n 2)
      1
      (* n (factorial (- n 1)))))

(defun power (base exp)
  (if (eq? exp 0)
      1
      (* base (power base (- exp 1)))))

(defun mod2pi (x)
  (- x (* (floor (/ x (* 2 3.141592653589793))) (* 2 3.141592653589793))))

; sine function using Maclaurin series approximation
(defun sin  (x)
  (let (x (mod2pi x))
  (+ x
    (- (/ (power x 3) (factorial 3)))
    (+ (/ (power x 5) (factorial 5)))
    (- (/ (power x 7) (factorial 7))))))