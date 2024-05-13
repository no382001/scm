```
  __   .__                                        ._.
_/  |_ |__|  ____  ___.__.  ______  ____    _____ | |
\   __\|  | /    \<   |  | /  ___/_/ ___\  /     \| |
 |  |  |  ||   |  \\___  | \___ \ \  \___ |  Y Y  \\|
 |__|  |__||___|  // ____|/____  > \___  >|__|_|  /__
                \/ \/          \/      \/       \/ \/ 

(eval x)             return evaluated x (such as when x was quoted)
(quote x)            special form, returns x unevaluated "as is"
(cons x y)           construct pair (x . y)
(car p)              car of pair p
(cdr p)              cdr of pair p
(add n1 n2 ... nk)   sum of n1 to nk
(sub n1 n2 ... nk)   n1 minus sum of n2 to nk
(mul n1 n2 ... nk)   product of n1 to nk
(div n1 n2 ... nk)   n1 divided by the product of n2 to nk
(int n)              integer part of n
(< n1 n2)            #t if n1<n2, otherwise ()
(eq? x y)            #t if x equals y, otherwise ()
(not x)              #t if x is (), otherwise ()
(or x1 x2 ... xk)    first x that is not (), otherwise ()
(and x1 x2 ... xk)   last x if all x are not (), otherwise ()
(cond (x1 y1)
        (x2 y2)
        ...
        (xk yk))       the first yi for which xi evaluates to non-()
(if x y z)           if x is non-() then y else z
(let* (v1 x1)
        (v2 x2)
        ...
        y)             sequentially binds each variable v1 to xi to evaluate y
(lambda v x)         construct a closure
(define v x)         define a named value globally 
(load 'filename)     read a file into stdin
(display 'string)    displays the evaluation result of an expression
(newline)            outputs a newline character
(begin
    (exp1) ...
    (exprn))        evaluates a sequence of expressions and returns the result of the last
(macro args body)   creates a macro
(setq name val)     sets the value of a variable in the local environment.
(read)              reads an expression from the input.
(setcar! list val)  sets the first element of a pair.
(setcdr! list val)  sets the second element of a pair.
(atom? v)           checks if arg is an atom
(number? v)         checks if arg is a number
(prim? v)           checks if arg is a prim-proc

(__gc)              triggers garbage collection.
(__trace on step)   toggles tracing of function calls and stepping through expressions
(__rcrbcs)          rolls back the callstack, used to simulate infinitely deep recursion
