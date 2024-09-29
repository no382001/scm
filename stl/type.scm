; included from core.scm

(defun type? (value expected-type)
  (cond ((eq? expected-type 'number_t) (number? value))
        ((eq? expected-type 'boolean_t) (or (eq? value #t) (eq? value #f)))
        ((eq? expected-type 'list_t) (list? value))
        (else #f)))

(define __type-env '())

; store the function's type signature in __type-env.
(defun store-type (name arg-types res-type)
  (setq __type-env (cons (cons name (cons arg-types res-type)) __type-env)))

(defun print-types (types)
  (map (lambda (type) (begin (display type) (newline))) types))

; splits a list on the given separator
(defun split-list-on (lst separator)
  (if (null? lst)
      '()
      (if (eq? (car lst) separator)
          (split-list-on (cdr lst) separator)
          (cons (car lst) (split-list-on (cdr lst) separator)))))

; parse a type signature separated by '->'. returns a list of argument types and the result type.
(defun parse-type-signature (sig)
  (let (split-sig (split-list-on sig '->))
    (if (null? split-sig)
        (display 'invalid-type-signature)
        (let (arg-types (butlast split-sig))
              (res-type (car (last split-sig))))
          (values arg-types res-type))))

;(split-list-on '(int -> char -> bool -> string) '->)

; (: name type1 -> type2 -> ... -> result_type)

; make a function impl that checks the arguments for the expected types

; make an alias for defun that goes over __type-env and since : will save the types in there
; if its a typed function, deduce the arguments

; what about -> it can be a cons cell so just stick with it as syntax for :
; how can i do type deduction? if i had typed primitive functions, i could do it pretty easy

