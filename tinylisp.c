/* tinylisp-opt.c with NaN boxing (optimized version) by Robert A. van Engelen 2022

BSD 3-Clause License

Copyright (c) 2021, Robert van Engelen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

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
#define N 1024
/* hp: heap pointer, A+hp with hp=0 points to the first atom string in cell[]
   sp: stack pointer, the stack starts at the top of cell[] with sp=N
   safety invariant: hp <= sp<<3 */
I hp = 0, sp = N;

/* atom, primitive, cons, closure and nil tags for NaN boxing */
I ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb, NIL = 0x7ffc;

/* errors */
typedef enum {
  NONE = 0,
  OUT_OF_MEMORY,
  DIVIDE_ZERO,
  PRIM_PROC_NOT_FOUND,
  CAR_NOT_A_PAIR,
  CDR_NOT_A_PAIR,
  ASSOC_VALUE_N_FOUND,
  EVAL_F_IS_NOT_A_FUNC,
  APPLY_F_IS_N_CLOS_OR_PRIM,
  TYPE_MISMATCH,
} ERROR_T;

#include "error_map.h"

typedef struct {
  ERROR_T type;
  L box;
  L proc;
} ERROR_STATE;

/* global error state 'eval' checks if its none and jumps to main, overridden in multiple layers */
static ERROR_STATE g_err_state = { NONE, 0, 0 };

jmp_buf jb;

/* cell[N] array of Lisp expressions, shared by the stack and atom heap */
L cell[N];

/* Lisp constant expressions () (nil), #t, ERR, and the global environment env */
L nil, tru, err, env;

/* NaN-boxing specific functions:
   box(t,i): returns a new NaN-boxed double with tag t and ordinal i
   ord(x):   returns the ordinal of the NaN-boxed double x
   num(n):   convert or check number n (does nothing, e.g. could check for NaN)
   equ(x,y): returns nonzero if x equals y */
L box(I t, I i) {
  L x;
  *(unsigned long long *)&x = (unsigned long long)t << 48 | i;
  return x;
}

/* the return value is narrowed to 32 bit unsigned integer to remove the tag */
I ord(L x) { return *(unsigned long long *)&x; }
L num(L n) { return n; }
I equ(L x, L y) { return *(unsigned long long *)&x == *(unsigned long long *)&y; }

/* interning of atom names (Lisp symbols), returns a unique NaN-boxed ATOM */
L atom(const char *s) {
  I i = 0;
  while (i < hp && strcmp(A + i, s)) /* search matching atom name */
    i += strlen(A + i) + 1;
  if (i == hp && (hp += strlen(strcpy(A + i, s)) + 1) > sp << 3) /* if not found, allocate and add a new atom name, abort when oom */
    abort();
  return box(ATOM, i);
}

/* construct pair (x . y) returns a NaN-boxed CONS */
L cons(L x, L y) {
  cell[--sp] = x; /* push car of x  */
  cell[--sp] = y; /* push cdr of y */
  if (hp > sp << 3)
    abort(); /* oom */
  return box(CONS, sp);
}

/* return the car of a pair or ERR if not a pair */
L car(L p) { 
  if ((T(p) & ~(CONS ^ CLOS)) == CONS ){
    return cell[ord(p) + 1];
  } else {
    g_err_state.type = CAR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

/* return the cdr of a pair or ERR if not a pair */
L cdr(L p) {
  if ((T(p) & ~(CONS ^ CLOS)) == CONS ){
    return cell[ord(p)];
  } else {
    g_err_state.type = CDR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

/* construct a pair to add to environment e, returns the list ((v . x) . e) */
L pair(L v, L x, L e) { return cons(cons(v, x), e); }

/* construct a closure, returns a NaN-boxed CLOS */
L closure(L v, L x, L e) { return box(CLOS, ord(pair(v, x, equ(e, env) ? nil : e))); }

/* look up a symbol in an environment, return its value or ERR if not found */
L assoc(L v, L e) {
  while (T(e) == CONS && !equ(v, car(car(e)))){
    e = cdr(e);
  }
  if (T(e) == CONS){
    return cdr(car(e));
  } else {
    g_err_state.type = ASSOC_VALUE_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}

/* _not(x) is nonzero if x is the Lisp () empty list */
I _not(L x) { return T(x) == NIL; }

/* let(x) is nonzero if x is a Lisp let/let* pair */
I let(L x) { return T(x) != NIL && (x = cdr(x), T(x) != NIL); }
L eval(L, L), parse();

/* return a new list of evaluated Lisp expressions t in environment e */
L evlis(L t, L e) {
  L s, *p;
  for (s = nil, p = &s; T(t) == CONS; p = cell + sp, t = cdr(t))
    *p = cons(eval(car(t), e), nil);
  if (T(t) == ATOM)
    *p = assoc(t, e);
  return s;
}

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
   (define v x)        define a named value globally */
L f_eval(L t, L *e) { return car(evlis(t, *e)); }
L f_quote(L t, L *_) { return car(t); }
L f_cons(L t, L *e) { return t = evlis(t, *e), cons(car(t), car(cdr(t))); }
L f_car(L t, L *e) { return car(car(evlis(t, *e))); }
L f_cdr(L t, L *e) { return cdr(car(evlis(t, *e))); }

/* make a macro for prim proc also check for isnan*/
L f_add(L t, L *e)
{
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n += car(t);
  return num(n);
}
L f_sub(L t, L *e)
{
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n -= car(t);
  return num(n);
}
L f_mul(L t, L *e)
{
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n *= car(t);
  return num(n);
}
L f_div(L t, L *e) {
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t))){
    L x = car(t);
    if (x == 0){
      g_err_state.type = DIVIDE_ZERO;
      g_err_state.box = t;
      return err;
    }
    n /= x;
  }

  return num(n);
}
L f_int(L t, L *e)
{
  L n = car(evlis(t, *e));
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}
L f_lt(L t, L *e) { return t = evlis(t, *e), car(t) - car(cdr(t)) < 0 ? tru : nil; }
L f_eq(L t, L *e) { return t = evlis(t, *e), equ(car(t), car(cdr(t))) ? tru : nil; }
L f_not(L t, L *e) { return _not(car(evlis(t, *e))) ? tru : nil; }
L f_or(L t, L *e)
{
  L x = nil;
  while (T(t) != NIL && _not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}
L f_and(L t, L *e)
{
  L x = nil;
  while (T(t) != NIL && !_not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}
L f_cond(L t, L *e)
{
  while (T(t) != NIL && _not(eval(car(car(t)), *e)))
    t = cdr(t);
  return car(cdr(car(t)));
}
L f_if(L t, L *e) { return car(cdr(_not(eval(car(t), *e)) ? cdr(t) : t)); }
L f_leta(L t, L *e)
{
  for (; let(t); t = cdr(t))
    *e = pair(car(car(t)), eval(car(cdr(car(t))), *e), *e);
  return car(t);
}
L f_lambda(L t, L *e) { return closure(car(t), car(cdr(t)), *e); }
L f_define(L t, L *e)
{
  env = pair(car(t), eval(car(cdr(t)), *e), env);
  return car(t);
}
/* table of Lisp primitives, each has a name s and function pointer f */
struct {
  const char *s;
  L (*f)
  (L, L *);
  short t;
} prim[] = {
  {"eval", f_eval, 1},
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
  {"lambda", f_lambda, 0},
  {"define", f_define, 0},
  {0}};

void print(L x);

/* TC opt eval */
L eval(L x, L e) {

  L f, v, d;
  while (1) {
    if (T(x) == ATOM) {
      return assoc(x, e);
    }
    if (T(x) != CONS) { /* could be a prim, return directly */
      return x;
    }
    L proc = x; /* save the proc for error message */
    f = eval(car(x), e); /* get proc sig */

    /* it can fail, like in the case of cons?? or equ() is just for numbers? */
    
    x = cdr(x); /* get proc body */
    if (T(f) == PRIM) {
      x = prim[ord(f)].f(x, &e); /* exec prim func */
      
      if (g_err_state.type) {
        if (!g_err_state.proc)
          g_err_state.proc = proc;
        longjmp(jb,1);
      }

      if (prim[ord(f)].t) /* do tc if its on */
        continue;
      return x;
    }
    
    if (T(f) != CLOS) {
      g_err_state.type = EVAL_F_IS_NOT_A_FUNC;
      g_err_state.box = car(proc);
      g_err_state.proc = proc;
      return err;
    }

    v = car(car(f)); /* list of params from clos */
    d = cdr(f); /* clos env */
    if (T(d) == NIL) { /* if clos env empty use the glob env */
      d = env;
    }

    /* map each arg to its corr param */
    for (; T(v) == CONS && T(x) == CONS; v = cdr(v), x = cdr(x)) {
      d = pair(car(v), eval(car(x), e), d);
    }

    /* if there are more args to proc, continue eval */
    if (T(v) == CONS) {
      x = eval(x, e);
    }

    /* pair remaining parameters with their arguments */
    for (; T(v) == CONS; v = cdr(v), x = cdr(x)) {
      d = pair(car(v), car(x), d);
    }

    /* eval body of clos in the new env */
    if (T(x) == CONS) {
      x = evlis(x, e);    
    } else if (T(x) != NIL) {
      x = eval(x, e);
    }

    /* param list not empty? */
    if (T(v) != NIL) {
      d = pair(v, x, d);
    }

    /* next expr */
    x = cdr(car(f));
    e = d;
  }
}

char buf[40], see = ' ';

void look() {
  int c = getchar();
  see = c;
  if (c == EOF)
    exit(0);
}

I seeing(char c) { return c == ' ' ? see > 0 && see <= c : see == c; }

char get() {
  char c = see;
  look();
  return c;
}

char scan() {
  int i = 0;
  while (seeing(' '))
    look();
  if (seeing('(') || seeing(')') || seeing('\''))
    buf[i++] = get();
  else
    do
      buf[i++] = get();
    while (i < 39 && !seeing('(') && !seeing(')') && !seeing(' '));
  return buf[i] = 0, *buf;
}

L Read() { return scan(), parse(); }

L list() {
  L t, *p;
  for (t = nil, p = &t;; *p = cons(parse(), nil), p = cell + sp)
  {
    if (scan() == ')')
      return t;
    if (*buf == '.' && !buf[1])
      return *p = Read(), scan(), t;
  }
}

L parse() {
  L n;
  int i;
  if (*buf == '(')
    return list();
  if (*buf == '\'')
    return cons(atom("quote"), cons(Read(), nil));
  return sscanf(buf, "%lg%n", &n, &i) > 0 && !buf[i] ? n : atom(buf);
}

void print(L);

void printlist(L t) {
  for (putchar('(');; putchar(' '))
  {
    print(car(t));
    if (_not(t = cdr(t)))
      break;
    if (T(t) != CONS)
    {
      printf(" . ");
      print(t);
      break;
    }
  }
  putchar(')');
}

void print(L x) {
  if (T(x) == NIL)
    printf("()");
  else if (T(x) == ATOM)
    printf("%s", A + ord(x));
  else if (T(x) == PRIM)
    printf("<%s>", prim[ord(x)].s);
  else if (T(x) == CONS)
    printlist(x);
  else if (T(x) == CLOS)
    printf("{%u}", ord(x));
  else
    printf("%.10lg", x);
}

void gc() { sp = ord(env); }

#ifndef FUNC_TEST

int main() {

  int i;
  nil = box(NIL, 0);
  err = atom("ERR");
  tru = atom("#t");
  env = pair(tru, tru, nil);
  for (i = 0; prim[i].s; ++i){
    env = pair(atom(prim[i].s), box(PRIM, i), env);
  }
  
  setjmp(jb);

  while (1) {
    if (g_err_state.type) {
      printf("%u: ",sp);
      printf("|%s| ",ERROR_T_to_string[g_err_state.type]);
      print(g_err_state.box); printf(" @ "); print(g_err_state.proc);  putchar('\n');
      g_err_state.type = NONE;
    }
    gc();
    printf("\n%u>", sp - hp / 8);
    L res = eval(Read(), env);
    if (!equ(err,res)){
      print(res);
    }
  }
}

#endif