#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TINYSCHEME
#define TINYSCHEME

/* we only need two types to implement a Lisp interpreter:
        I    unsigned integer (either 16 bit, 32 bit or 64 bit unsigned)
        L    Lisp expression (double with NaN boxing)
   I variables and function parameters are named as follows:
        i    any unsigned integer, e.g. a NaN-boxed ordinal value
        t    a NaN-boxing tag
   L variables and function parameters are named as follows:
        x,y  any Lisp expression
        n    number
        t    list
        f    function or Lisp primitive
        p    pair, a cons of two Lisp expressions
        e,d  environment, a list of pairs, e.g. created with (define v x)
        v    the name of a variable (an atom) or a list of variables */
#define I unsigned
#define L double

/* T(x) returns the tag bits of a NaN-boxed Lisp expression x */
#define T(x) *(unsigned long long *)&x >> 48

/* address of the atom heap is at the bottom of the cell stack */
#define A (char *)cell

/* number of cells for the shared stack and atom heap, increase N as desired */
#define N 1024 * 20
/* hp: heap pointer, A+hp with hp=0 points to the first atom string in cell[]
   sp: stack pointer, the stack starts at the top of cell[] with sp=N
   safety invariant: hp <= sp<<3 */
I hp = 0, sp = N;

/* atom, primitive, cons, closure and nil tags for NaN boxing */
I ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb, NIL = 0x7ffc,
  MACR = 0x7ffd, NOP = 0x7ffe;

/* errors */
typedef enum {
  NONE = 0,
  STACK_HEAP_COLLISION,
  DIVIDE_ZERO,
  PRIM_PROC_NOT_FOUND,
  CAR_NOT_A_PAIR,
  CDR_NOT_A_PAIR,
  ASSOC_VALUE_N_FOUND,
  EVAL_F_IS_NOT_A_FUNC,
  APPLY_F_IS_N_CLOS_OR_PRIM,
  TYPE_MISMATCH,
  FUNCTION_DEF_IS_NOT_LAMBDA,
  LOAD_FILENAME_MUST_BE_QUOTED,
  LOAD_CANNOT_OPEN_FILE,
  LOAD_FAILED_TO_REDIRECT_STDIN,
  DISPLAY_NO_ARG,
  DISPLAY_EVAL_ERROR,
  BEGIN_NO_RETURN_VAL,
  NEWLINE_TAKES_NO_ARG,
  MACRO_EXPAND_BIND_FAILED,
  MACRO_EXPAND_BODY_EVAL_FAILED,
  MACRO_EXPAND_EXECUTION_FAILED,
  LOAD_OPEN_FILE_LIMIT_REACHED,
  SETQ_VAR_N_FOUND,
  SETCAR_ARG_NOT_CONS,
  SETCDR_ARG_NOT_CONS
} ERROR_T;

#include "util/error_map.h"

typedef struct {
  ERROR_T type;
  L box;
  L proc;
} ERROR_STATE;

/* global error state 'eval' checks if its none and jumps to main, overridden in
 * multiple layers */
static ERROR_STATE g_err_state = {NONE, 0, 0};

/* jump buffer used for exiting into main upon error */
jmp_buf jb;

/* used by f_load and f_load_close_streams  */
#define PARSE_CTX_BUFF_SIZE 1024
typedef struct {
  FILE *file;
  char buffer[PARSE_CTX_BUFF_SIZE];
  int buf_pos;
  int buf_end;
  char see;
} parsing_ctx;

// main sets file to *stdin
parsing_ctx default_ctx = {
    .file = NULL, .buffer = {0}, .buf_pos = 0, .buf_end = 0, .see = ' '};
parsing_ctx *curr_ctx = &default_ctx;

// used by __rcsoc
struct {
  L x;
  L e;
} rcso_struct;

/* used in f_define to ignore longjump and instead roll the error back to the
 * function */
short define_underway = 0;

/* trace things */
int trace_depth = 0;
char trace = 0;
char stepping = 0;
L step(L x, L e);
L f_trace(L x, L *e);

/* cell[N] array of Lisp expressions, shared by the stack and atom heap */
L cell[N];

/* Lisp constant expressions () (nil), #t, ERR, and the global environment env
 */
L nil, tru, nop, err, env;

#define PARSE_BUFFER 1024
char buf[PARSE_BUFFER];

void print(L);
int print_and_reset_error();

void switch_ctx_to_file(FILE *file);
void switch_ctx_to_stdin();

/* NaN-boxing specific functions:
   box(t,i): returns a new NaN-boxed double with tag t and ordinal i
   ord(x):   returns the ordinal of the NaN-boxed double x
   num(n):   convert or check number n (does nothing, e.g. could check for NaN)
   equ(x,y): returns nonzero if x equals y */
L box(I t, I i);

/* the return value is narrowed to 32 bit unsigned integer to remove the tag */
I ord(L x);
L num(L n);
I equ(L x, L y);

/* interning of atom names (Lisp symbols), returns a unique NaN-boxed ATOM */
L atom(const char *s);
L macro(L v, L x);

/* construct pair (x . y) returns a NaN-boxed CONS */
L cons(L x, L y);

/* return the car of a pair or ERR if not a pair */
L car(L p);

/* return the cdr of a pair or ERR if not a pair */
L cdr(L p);

/* construct a pair to add to environment e, returns the list ((v . x) . e) */
L pair(L v, L x, L e);

/* construct a closure, returns a NaN-boxed CLOS */
L closure(L v, L x, L e);

/* look up a symbol in an environment, return its value or ERR if not found */
L assoc(L v, L e);

/* _not(x) is nonzero if x is the Lisp () empty list */
I _not(L x);

/* let(x) is nonzero if x is a Lisp let/let* pair */
I let(L x);

/* return a new list of evaluated Lisp expressions t in environment e */
L evlis(L t, L e);

L f_eval(L t, L *e);
L f_quote(L t, L *_);
L f_cons(L t, L *e);
L f_car(L t, L *e);
L f_cdr(L t, L *e);

/* make a macro for prim proc also check for isnan*/
L f_add(L t, L *e);
L f_sub(L t, L *e);
L f_mul(L t, L *e);
L f_div(L t, L *e);
L f_int(L t, L *e);
L f_lt(L t, L *e);
L f_eq(L t, L *e);
L f_not(L t, L *e);
L f_or(L t, L *e);
L f_and(L t, L *e);
L f_cond(L t, L *e);
L f_if(L t, L *e);
L f_leta(L t, L *e);
L f_letreca(L t, L *e);
L f_let(L t, L *e);

// incorrectly defined lambdas abort
L f_lambda(L t, L *e);
// procedures can only be defined with lambdas
L f_define(L t, L *e);
void f_load_close_streams();

/* load expressions from file into stdin

   saves the original stdin and redirects the open file to stdin,
   uses a stack to keep trace of the open files,
   once stdin reaches EOF the filestream closes,
   and if there is no more files open the original stdin gets restored */
L f_load(L t, L *e);

/* prints an atom to stdout and returns error (but no err status set) */
L f_display(L t, L *e);
L f_newline(L t, L *e);

/* evaluates a list of exprs and returns the last value */
L f_begin(L t, L *e);
L f_macro(L t, L *e);
L f_setq(L t, L *e);
L f_read(L t, L *e);
L f_gc(L x, L *e);

// (set-car! pair expr) sets the value of the car cell of a cons pair to expr as
// a side-effect
L f_setcar(L t, L *e);

// (set-cdr! pair expr) sets the value of the cdr cell of a cons pair to expr as
// a side-effect.
L f_setcdr(L t, L *e);

// recursive-call-roll-back-call-stack
// workaround that allows arbitrary depth evals (cs doesnt blow)
// there is no way to exit currently, maybe with an error
L f_rcrbcs(L x, L *e);

L f_atomq(L x, L *e);
L f_numberq(L x, L *e);
L f_primq(L x, L *e);

void print(L x);
L eval(L x, L e);
void look();
I seeing(char c);
char get();
char scan();
L Read();
L list();
L parse();
void printlist(L t);
void gc();

/* Lisp primitives:
   (eval x)            return evaluated x (such as when x was quoted)
   (quote x)           special form, returns x unevaluated "as is"
   (cons x y)          construct pair (x . y)
   (car p)             car of pair p
   (cdr p)             cdr of pair p
   (add n1 n2 ... nk)  sum of n1 to nk
   (sub n1 n2 ... nk)  n1 minus sum of n2 to nk
   (mul n1 n2 ... nk)  product of n1 to nk
   (div n1 n2 ... nk)  n1 divided by the product of n2 to nk
   (int n)             integer part of n
   (< n1 n2)           #t if n1<n2, otherwise ()
   (eq? x y)           #t if x equals y, otherwise ()
   (not x)             #t if x is (), otherwise ()
   (or x1 x2 ... xk)   first x that is not (), otherwise ()
   (and x1 x2 ... xk)  last x if all x are not (), otherwise ()
   (cond (x1 y1)
         (x2 y2)
         ...
         (xk yk))      the first yi for which xi evaluates to non-()
   (if x y z)          if x is non-() then y else z
   (let* (v1 x1)
         (v2 x2)
         ...
         y)            sequentially binds each variable v1 to xi to evaluate y
   (lambda v x)        construct a closure
   (define v x)        define a named value globally
   (load filename)     read a file into stdin */

/* table of Lisp primitives, each has a name s and function pointer f */
struct {
  const char *s;
  L(*f)
  (L, L *);
  short t;
} prim[] = {{"eval", f_eval, 1},
            {"quote", f_quote, 0},
            {"cons", f_cons, 0},
            {"car", f_car, 0},
            {"cdr", f_cdr, 0},
            {"+", f_add, 0},
            {"-", f_sub, 0},
            {"*", f_mul, 0},
            {"/", f_div, 0},
            {"int", f_int, 0},
            {"<", f_lt, 0},
            {"eq?", f_eq, 0},
            {"or", f_or, 0},
            {"and", f_and, 0},
            {"not", f_not, 0},
            {"cond", f_cond, 1},
            {"if", f_if, 1},
            {"let*", f_leta, 1},
            {"let", f_let, 1},
            {"lambda", f_lambda, 0},
            {"define", f_define, 0},
            {"load", f_load, 0},
            {"display", f_display, 0},
            {"newline", f_newline, 0},
            {"begin", f_begin, 0},
            {"letrec*", f_letreca, 0},
            {"macro", f_macro, 0},
            {"setq", f_setq, 0},
            {"__trace", f_trace, 0},
            {"__gc", f_gc, 0},
            {"__rcrbcs", f_rcrbcs, 0},
            {"read", f_read, 0},
            {"set-car!", f_setcar, 0},
            {"set-cdr!", f_setcar, 0},
            {"atom?", f_atomq, 0},
            {"prim?", f_primq, 0},
            {"number?", f_numberq, 0},
            {0}};

#endif