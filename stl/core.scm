; core.scm

(define list (lambda args args))
(define defun (macro (f v x) (list 'define f (list 'lambda v x))))

(defun member (x t)
        (if t
            (if (eq? x (car t))
                #t
                (member x (cdr t)))
            #f))

; loads files if they havent been loaded before
(define __files-included '())
(defun include (f)
    (letrec* (include-impl (lambda (filename)
        (if (member filename __files-included)
            (begin
                (display filename)
                (display 'already-included-)
                (newline))
            (begin
                (display 'Loading-)
                (display filename)
                (newline)
                (setq __files-included (cons filename __files-included))
                (load filename)
                (display 'Loaded-)
                (display filename)
                (newline)))))
    (include-impl f)))

(include 'stl/common.scm)
(include 'stl/list.scm)
(include 'stl/type.scm)