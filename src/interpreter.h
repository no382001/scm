#pragma once
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

typedef unsigned tag_t;
typedef double expr_t;

#define A (char *)cell
#define N (1024 * 20)

extern expr_t cell[N];
extern tag_t hp, sp;
extern tag_t ATOM, PRIM, CONS, CLOS, NIL, MACR, NOP, VECTOR;
extern expr_t nil, tru, nop, err, env;

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
  expr_t box;
  expr_t proc;
} ERROR_STATE;

extern ERROR_STATE g_err_state;
extern jmp_buf jb;

typedef struct {
  const char *s;
  expr_t (*f)(expr_t, expr_t *);
  short t;
} prim_procs_t;

extern prim_procs_t prim[];

extern int suppress_jumps, trace_depth, trace, stepping;

typedef struct {
  expr_t x;
  expr_t e;
} rcso_struct_t;
extern rcso_struct_t rcso_ctx;

/* interpreter things */

expr_t T(expr_t x);
expr_t box(tag_t t, tag_t i);
tag_t ord(expr_t x);
expr_t num(expr_t n);
tag_t equ(expr_t x, expr_t y);
expr_t atom(const char *s);
expr_t macro(expr_t v, expr_t x);
expr_t cons(expr_t x, expr_t y);
expr_t car(expr_t p);
expr_t cdr(expr_t p);
expr_t pair(expr_t v, expr_t x, expr_t e);
expr_t closure(expr_t v, expr_t x, expr_t e);
expr_t assoc(expr_t v, expr_t e);
tag_t _not(expr_t x);
tag_t let(expr_t x);
expr_t evlis(expr_t t, expr_t e);

expr_t vector(tag_t size);
expr_t vector_ref(expr_t vec, tag_t index);
expr_t vector_set(expr_t vec, tag_t index, expr_t value);
expr_t vector_length(expr_t vec);
expr_t car(expr_t p);
expr_t cdr(expr_t p);
expr_t pair(expr_t v, expr_t x, expr_t e);
expr_t closure(expr_t v, expr_t x, expr_t e);
expr_t assoc(expr_t v, expr_t e);
tag_t _not(expr_t x);
tag_t let(expr_t x);
expr_t evlis(expr_t t, expr_t e);
expr_t bind(expr_t v, expr_t t, expr_t e);
expr_t expand(expr_t f, expr_t t, expr_t e);
expr_t eval(expr_t x, expr_t e);
expr_t step(expr_t x, expr_t e);
void gc();
int print_and_reset_error();

/* prim procs */

expr_t f_eval(expr_t t, expr_t *e);
expr_t f_quote(expr_t t, expr_t *_);
expr_t f_cons(expr_t t, expr_t *e);
expr_t f_car(expr_t t, expr_t *e);
expr_t f_cdr(expr_t t, expr_t *e);
expr_t f_add(expr_t t, expr_t *e);
expr_t f_sub(expr_t t, expr_t *e);
expr_t f_mul(expr_t t, expr_t *e);
expr_t f_div(expr_t t, expr_t *e);
expr_t f_int(expr_t t, expr_t *e);
expr_t f_lt(expr_t t, expr_t *e);
expr_t f_eq(expr_t t, expr_t *e);
expr_t f_not(expr_t t, expr_t *e);
expr_t f_or(expr_t t, expr_t *e);
expr_t f_and(expr_t t, expr_t *e);
expr_t f_cond(expr_t t, expr_t *e);
expr_t f_if(expr_t t, expr_t *e);
expr_t f_leta(expr_t t, expr_t *e);
expr_t f_letreca(expr_t t, expr_t *e);
expr_t f_let(expr_t t, expr_t *e);
expr_t f_lambda(expr_t t, expr_t *e);
expr_t f_define(expr_t t, expr_t *e);
expr_t f_macro(expr_t t, expr_t *e);
expr_t f_load(expr_t t, expr_t *e);
expr_t f_display(expr_t t, expr_t *e);
expr_t f_newline(expr_t t, expr_t *e);
expr_t f_begin(expr_t t, expr_t *e);
expr_t f_setq(expr_t t, expr_t *e);
expr_t f_trace(expr_t x, expr_t *e);
expr_t f_read(expr_t t, expr_t *e);
expr_t f_gc(expr_t x, expr_t *e);
expr_t f_rcrbcs(expr_t x, expr_t *e);
expr_t f_setcar(expr_t t, expr_t *e);
expr_t f_setcdr(expr_t t, expr_t *e);
expr_t f_atomq(expr_t x, expr_t *e);
expr_t f_numberq(expr_t x, expr_t *e);
expr_t f_primq(expr_t x, expr_t *e);
expr_t f_vector(expr_t t, expr_t *e);
expr_t f_list_primitives(expr_t t, expr_t *e);
expr_t f_vector_ref(expr_t t, expr_t *e);
expr_t f_vector_set(expr_t t, expr_t *e);
expr_t f_vector_length(expr_t t, expr_t *e);
expr_t f_unquote(expr_t t, expr_t *e);
expr_t f_quasiquote(expr_t t, expr_t *e);
expr_t eval_quasiquote(expr_t x, int level);

/* builder */

#include "parser.h"

expr_t read();
prim_t look();
tag_t seeing(char c);
prim_t get();
prim_t scan();
expr_t list();
expr_t parse_number_or_atom(const char *buf, int offset);
expr_t parse();