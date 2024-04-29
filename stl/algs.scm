(define do (lambda  (i step body condition update result)
    (letrec* (loop
            (lambda (i)
                (if (condition i)
                    (result i)
                    (begin
                        (body i)
                        (loop (update i step))))))
    (loop i))))

; count from 0 to 11
;(do 0 1 (lambda (i) (begin (display i) (newline))) (lambda (i) (< 10 i)) (lambda (i step) (+ i step)) (lambda (i) i))

(define do-collect (lambda (i step init-list body condition update result)
    (letrec* (loop
            (lambda (i lst)
                (if (condition i)
                    (result lst)
                    (loop (update i step) (cons (body i) lst)))))
    (loop i init-list))))


(do-collect 0 1 '()   ; start from 0, empty initial list
  (lambda (i) i)                  ; body: the number itself
  (lambda (i) (eq? i 10))          ; condition: stop when i is 10
  (lambda (i step) (+ i step))    ; update: increment i by step
  (lambda (lst) (reverse lst)))  ; result: return the reversed list to maintain order
