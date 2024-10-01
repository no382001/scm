#pragma once
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#define I unsigned
#define L double
#define A (char *)cell
#define N (1024 * 20)

extern L cell[N];
extern I hp, sp;
extern I ATOM, PRIM, CONS, CLOS, NIL, MACR, NOP, VECTOR;
extern L nil, tru, nop, err, env;

#include "../util/error_map.h"

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
  SETCDR_ARG_NOT_CONS,
  VECTOR_FN_NOT_A_VECTOR,
  VECTOR_FN_INDEX_OOB,
  UNQUOTE_OUTSIDE_QUASIQUOTE
} ERROR_T;

typedef struct {
  ERROR_T type;
  L box;
  L proc;
} ERROR_STATE;

extern ERROR_STATE g_err_state;
extern jmp_buf jb;

typedef struct {
  const char *s;
  L(*f)
  (L, L *);
  short t;
} prim_procs_t;

extern prim_procs_t prim[];

extern int suppress_jumps, trace_depth, trace, stepping;

typedef struct {
  L x;
  L e;
} rcso_struct_t;
extern rcso_struct_t rcso_ctx;

/* interpreter things */

L T(L x);
L box(I t, I i);
I ord(L x);
L num(L n);
I equ(L x, L y);
L atom(const char *s);
L macro(L v, L x);
L cons(L x, L y);
L car(L p);
L cdr(L p);
L pair(L v, L x, L e);
L closure(L v, L x, L e);
L assoc(L v, L e);
I _not(L x);
I let(L x);
L evlis(L t, L e);

L vector(I size);
L vector_ref(L vec, I index);
L vector_set(L vec, I index, L value);
L vector_length(L vec);
L car(L p);
L cdr(L p);
L pair(L v, L x, L e);
L closure(L v, L x, L e);
L assoc(L v, L e);
I _not(L x);
I let(L x);
L evlis(L t, L e);
L bind(L v, L t, L e);
L expand(L f, L t, L e);
L eval(L x, L e);
L step(L x, L e);
void gc();
int print_and_reset_error();

/* prim procs */

L f_eval(L t, L *e);
L f_quote(L t, L *_);
L f_cons(L t, L *e);
L f_car(L t, L *e);
L f_cdr(L t, L *e);
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
L f_lambda(L t, L *e);
L f_define(L t, L *e);
L f_macro(L t, L *e);
L f_load(L t, L *e);
L f_display(L t, L *e);
L f_newline(L t, L *e);
L f_begin(L t, L *e);
L f_setq(L t, L *e);
L f_trace(L x, L *e);
L f_read(L t, L *e);
L f_gc(L x, L *e);
L f_rcrbcs(L x, L *e);
L f_setcar(L t, L *e);
L f_setcdr(L t, L *e);
L f_atomq(L x, L *e);
L f_numberq(L x, L *e);
L f_primq(L x, L *e);
L f_vector(L t, L *e);
L f_list_primitives(L t, L *e);
L f_vector_ref(L t, L *e);
L f_vector_set(L t, L *e);
L f_vector_length(L t, L *e);
L f_unquote(L t, L *e);
L f_quasiquote(L t, L *e);
L eval_quasiquote(L x, int level);

/* builder */

#include "parser.h"

L read();
prim_t look();
I seeing(char c);
prim_t get();
prim_t scan();
L list();
L parse_number_or_atom(const char *buf, int offset);
L parse();