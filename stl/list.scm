; included by core.scm

(defun length (t)
    (if t
        (+ 1 (length (cdr t)))
        0))

(defun append1 (s t)
    (if s
        (cons (car s) (append1 (cdr s) t))
        t))

(defun append (t . args)
    (if args
        (append1 t (apply append args))
        t))

(defun nthcdr (t n)
    (if (eq? n 0)
        t
        (nthcdr (cdr t) (- n 1))))

(defun nth (t n)
    (car (nthcdr t n)))

(defun rev1 (r t)
    (if t
        (rev1 (cons (car t) r) (cdr t))
        r))

(defun reverse (t)
    (rev1 () t))

(defun foldr (f x t)
    (if t
        (f (car t) (foldr f x (cdr t)))
        x))

(defun foldl (f x t)
    (if t
        (foldl f (f (car t) x) (cdr t))
        x))

(defun min (args)
    (foldl
        (lambda (x y)
            (if (< x y)
                x
                y))
        9.9999999
        args))

(defun max (args)
    (foldl (lambda (x y)
        (if (< x y)
            y
            x))
    -9.9999999
    args))

(defun filter (f t)
    (if t
        (if (f (car t))
            (cons (car t) (filter f (cdr t)))
            (filter f (cdr t)))
        ()))

(defun all? (f t)
    (if t
        (and
            (f (car t))
            (all? f (cdr t)))
        #t))

(defun any? (f t)
    (if t
        (or
            (f (car t))
            (any? f (cdr t)))
        ()))

(defun mapcar (f t)
    (if t
        (cons (f (car t)) (mapcar f (cdr t)))
        ()))

(defun map (f . args)
    (if (any? null? args)
        ()
        (let*
            ((x (mapcar car args))
             (t (mapcar cdr args)))
            (cons (apply f x) (apply map (cons f t))))))

(defun zip (args)
    (apply map list args))

(defun seq (n m)
    (if (< n m)
        (cons n (seq (+ n 1) m))
        ()))

(defun seqby (n m k)
    (if (< 0 (* k (- m n)))
        (cons n (seqby (+ n k) m k))
        ()))

(defun range (n m . args)
    (if args
        (seqby n m (car args))
        (seq n m)))
