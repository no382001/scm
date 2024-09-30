#pragma once

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
L type(L x) { return *(unsigned long long *)&x >> 48; }

/* address of the atom heap is at the bottom of the cell stack */
#define A (char *)cell
/* number of cells for the shared stack and atom heap, increase N as desired */
#define N (1024 * 20)
L cell[N];
I hp = 0, sp = N;

/* atom, primitive, cons, closure and nil tags for NaN boxing */
I ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb, NIL = 0x7ffc,
  MACR = 0x7ffd, NOP = 0x7ffe, VECTOR = 0x7fff;

L nil, tru, nop, err, env;