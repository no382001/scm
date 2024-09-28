; included from core.scm

; for cond
(define else #t)

(defun type? (value expected-type)
  (cond ((eq? expected-type 'number_t) (number? value))
        ((eq? expected-type 'boolean_t) (or (eq? value #t) (eq? value #f)))
        ((eq? expected-type 'list_t) (list? value))
        (else #f)))

(type? 3 'number_t)
(type? else 'number_t)

(type? #f 'boolean_t)
(type? 11 'boolean_t)

(type? '(1 2 3) 'list_t)
;(type? (1 2 3) 'list_t) ; if i fault here the reader goes into a frenzy 16944: |EVAL_F_IS_NOT_A_FUNC| 1 @ (1 2 3)


(define __type-env '())
; i need to support multiple things
; split the shit for the argument count
; i need ` and , to progress on this

; make a function impl that checks the arguments for the expected types

; make an alias for defun that goes over __type-env and since : will save the types in there
; if its a typed function, deduce the arguments

; what about -> it can be a cons cell so just stick with it as syntax for :
; how can i do type deduction? if i had typed primitive functions, i could do it pretty easy

