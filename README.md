```
  __   .__                                        ._.
_/  |_ |__|  ____  ___.__.  ______  ____    _____ | |
\   __\|  | /    \<   |  | /  ___/_/ ___\  /     \| |
 |  |  |  ||   |  \\___  | \___ \ \  \___ |  Y Y  \\|
 |__|  |__||___|  // ____|/____  > \___  >|__|_|  /__
                \/ \/          \/      \/       \/ \/

based on Robert-van-Engelen's 'tinylisp: lisp in 99 lines of C' that uses NaN boxing, its 'almost' usable...

evaluation and quoting
(eval x env)                            - returns the evaluated result of x.
(quote x)                               - special form, returns x unevaluated "as is".
(unquote x)                             - returns the value of x inside a quasiquote.
(quasiquote x)                          - similar to quote, but allows unquote.

cons and vector operations
(cons x y)                              - constructs a pair (x . y).
(car p)                                 - returns the first element of the pair p.
(cdr p)                                 - returns the second element of the pair p.
(setcar! list val)                      - sets the first element of a pair.
(setcdr! list val)                      - sets the second element of a pair.
(vector ... )                           - creates a vector.
(vector-ref vec i)                      - accesses the ith element of a vector.
(vector-set! vec i val)                 - sets the ith element of the vector to val.
(vector-length vec)                     - returns the length of the vector.

arithmetic operations
(add n1 n2 ... nk)                      - sum of n1 to nk.
(sub n1 n2 ... nk)                      - n1 minus the sum of n2 to nk.
(mul n1 n2 ... nk)                      - product of n1 to nk.
(div n1 n2 ... nk)                      - n1 divided by the product of n2 to nk.
(int n)                                 - returns the integer part of n.

logical and comparison operations
(< n1 n2)                               - #t if n1 < n2, otherwise ().
(eq? x y)                               - #t if x equals y, otherwise ().
(not x)                                 - #t if x is (), otherwise ().
(or x1 x2 ... xk)                       - first x that is not (), otherwise ().
(and x1 x2 ... xk)                      - last x if all x are not (), otherwise ().

conditional and control flow
(cond (x1 y1)
      (x2 y2)
      ...
      (xk yk))                          - returns the first yi for which xi evaluates to non-().
(if x y z)                              - if x is non-() then returns y, otherwise returns z.

variable binding and functions
(let* (v1 x1)
      (v2 x2)
      ...
      y)                                - sequentially binds each variable v1 to xi to evaluate y.
(lambda v x)                            - constructs a closure.
(define v x)                            - defines a named value v globally.

i/o operations
(load 'filename)                        - reads and sequentially evaluates a file.
(read)                                  - reads something from stdin
(display 'string)                       - displays the evaluation result of an expression.
(newline)                               - outputs a newline character.

sequence
(begin
     (exp1) ...
     (exprn))                            - evaluates a sequence of expressions and returns the result of the last.

macros and assignment
(macro args body)                       - creates a macro. non-hygenic?
(setq name val)                         - sets the value of a variable in the local environment.

type checking
(atom? v)                               - checks if v is an atom.
(number? v)                             - checks if v is a number.
(prim? v)                               - checks if v is a primitive procedure.

garbage collection and debugging
(__gc)                                  - triggers garbage collection. (you can only do this manually)
(__trace on step)                       - toggles tracing of function calls and stepping through expressions.
(__rcrbcs)                              - rolls back the call stack to simulate infinitely deep recursion.
```