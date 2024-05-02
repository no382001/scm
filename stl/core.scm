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

(defun do (i step body condition update result)
    (letrec* (loop
            (lambda (i)
                (if (condition i)
                    (result i)
                    (begin
                        (body i)
                        (loop (update i step))))))
    (loop i)))

;count from 0 to 11
;(do 0 1 (lambda (i) (begin (display i) (newline))) (lambda (i) (< 10 i)) (lambda (i step) (+ i step)) (lambda (i) i))

(defun do-collect (i step init-list body condition update result)
    (letrec* (loop
            (lambda (i lst)
                (if (condition i)
                    (result lst)
                    (loop (update i step) (cons (body i) lst)))))
    (loop i init-list)))
#|

(include stl/list.scm)
(do-collect 0 1 '()   ; start from 0, empty initial list
  (lambda (i) i)                  ; body: the number itself
  (lambda (i) (eq? i 10))          ; condition: stop when i is 10
  (lambda (i step) (+ i step))    ; update: increment i by step
  (lambda (lst) (reverse lst)))  ; result: return the reversed list to maintain order
|#

(define repl (lambda ()
    (begin
        (newline)
        (display '-->)
        (display
            (eval
                (begin
                    (define repl-counter (+ 1 repl-counter)
                    repl-counter))))
        (repl))))
;(repl)