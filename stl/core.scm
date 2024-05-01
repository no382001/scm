; core.scm
(define list (lambda args args))
(define defun (macro (f v x) (list 'define f (list 'lambda v x))))
