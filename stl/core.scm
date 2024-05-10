; core.scm

(define list (lambda args args))
(define defun (macro (f v x) (list 'define f (list 'lambda v x))))

;returns the rest of list 't' if 'x' is a member
;return () if it is not
(define member
    (lambda (x t)
        (if t
            (if (eq? x (car t))
                t
                (member x (cdr t)))
            t)))

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


;(__trace 1 1)

(define repl-counter 0)
(define repl (lambda ()
    (__rcrbcs (begin
        (newline)
        (display '-->)
        (display
            (eval
                (begin
                    (setq repl-counter (+ 1 repl-counter)
                    repl-counter))))
        (repl)))))

(repl)