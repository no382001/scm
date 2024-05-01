(load stl/common.scm) ; defun
(load prog/toy.scm)

(defun loopreadprint ()
    (begin
        (display (read))
        (newline)
        (loopreadprint)))
