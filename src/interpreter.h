#pragma once
#include <errno.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define container_of(ptr, type, member)                                        \
  ({                                                                           \
    const typeof(((type *)0)->member) *__mptr = (ptr);                         \
    (type *)((char *)__mptr - offsetof(type, member));                         \
  })

typedef unsigned tag_t;
typedef double expr_t;

extern const tag_t ATOM, PRIM, CONS, CLOS, NIL, MACR, NOP, VECTOR;

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
} error_code_t;

typedef struct {
  error_code_t type;
  expr_t box;
  expr_t proc;
} error_state_t;

#define N (1024 * 20)

typedef struct {
  expr_t cells[N];
  tag_t hp;
  tag_t sp;
} memory_t;

typedef struct {
  expr_t nil;
  expr_t tru;
  expr_t nop;
  expr_t err;
} constants_t;

typedef struct {
  int trace_depth;
  bool stepping;
  bool trace;
} trace_ctx_t;

typedef struct interpreter_t interpreter_t;

typedef struct {
  const char *s;
  expr_t (*f)(expr_t, expr_t *, interpreter_t *);
  short t;
} prim_procs_t;

#define ctx_env ctx->internals.env

typedef struct {
  expr_t env;
  prim_procs_t *prims;
} internal_t;

typedef struct {
  expr_t x;
  expr_t e;
} rcso_ctx_t;

typedef struct {
  jmp_buf jb;
  int suppress_jumps;
  rcso_ctx_t rcso;
} jumping_around_t;

#define llc_cell ctx->memory.cells
#define llc_atomheap (char *)llc_cell
#define llc_hp ctx->memory.hp
#define llc_sp ctx->memory.sp

#define llc_c_nil ctx->constants.nil
#define llc_c_err ctx->constants.err

#define llc_jb ctx->jumps.jb

#define llc_errstate ctx->error

typedef struct {
  memory_t memory;
  constants_t constants;
  jumping_around_t jumps;
  error_state_t error;
} low_level_ctx_t;

#define ic_cell ctx->llc.memory.cells
#define ic_atomheap (char *)ic_cell
#define ic_hp ctx->llc.memory.hp
#define ic_sp ctx->llc.memory.sp

#define ic_c_nil ctx->llc.constants.nil
#define ic_c_tru ctx->llc.constants.tru
#define ic_c_nop ctx->llc.constants.nop

#define ic_errstate ctx->llc.error
#define ic_c_err ctx->llc.constants.err

#define ic_trace ctx->trace

#define ic_jumps ctx->llc.jumps
#define ic_jb ic_jumps.jb

#define ic_env ctx->internals.env
#define ic2llcref &ctx->llc

#define ic_rcso ic_jumps.rcso
#define ic_prim ctx->internals.prims

struct interpreter_t {
  low_level_ctx_t llc;
  trace_ctx_t trace;
  internal_t internals;
  bool nosetjmp; // setjmp is set externally
  bool noprint;
  bool noreseterr;
};

//#include "../util/error_map.h"

expr_t T(expr_t x);

/* interpreter things */
expr_t box(tag_t t, tag_t i);
tag_t ord(expr_t x);
expr_t num(expr_t n);
tag_t equ(expr_t x, expr_t y);
expr_t atom(const char *s, low_level_ctx_t *ctx);
expr_t macro(expr_t v, expr_t x, low_level_ctx_t *ctx);
expr_t cons(expr_t x, expr_t y, low_level_ctx_t *ctx);
expr_t vector(tag_t size, low_level_ctx_t *ctx);
expr_t vector_ref(expr_t vec, tag_t index, low_level_ctx_t *ctx);
expr_t vector_set(expr_t vec, tag_t index, expr_t value, low_level_ctx_t *ctx);
expr_t vector_length(expr_t vec, low_level_ctx_t *ctx);
expr_t car(expr_t p, low_level_ctx_t *ctx);
expr_t cdr(expr_t p, low_level_ctx_t *ctx);
expr_t pair(expr_t v, expr_t x, expr_t e, low_level_ctx_t *ctx);
expr_t assoc(expr_t v, expr_t e, low_level_ctx_t *ctx);
tag_t _not(expr_t x);
tag_t let(expr_t x, low_level_ctx_t *ctx);
expr_t bind(expr_t v, expr_t t, expr_t e, low_level_ctx_t *ctx);

expr_t closure(expr_t v, expr_t x, expr_t e, interpreter_t *ctx);
expr_t expand(expr_t f, expr_t t, expr_t e, interpreter_t *ctx);
expr_t evlis(expr_t t, expr_t e, interpreter_t *ctx);
expr_t eval(expr_t x, expr_t e, interpreter_t *ctx);
expr_t step(expr_t x, expr_t e, interpreter_t *ctx);

void gc(interpreter_t *ctx);
int print_and_reset_error(interpreter_t *ctx);

/* prim procs */
#include "interpreter.h"
#include "print.h"

expr_t f_eval(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_quote(expr_t t, expr_t *_, interpreter_t *ctx);
expr_t f_cons(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_car(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_cdr(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_add(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_sub(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_mul(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_div(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_int(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_lt(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_eq(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_not(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_or(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_and(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_cond(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_if(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_leta(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_letreca(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_let(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_lambda(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_define(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_macro(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_load(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_display(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_newline(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_begin(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_setq(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_trace(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_read(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_gc(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_rcrbcs(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_setcar(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_setcdr(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_atomq(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_numberq(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_primq(expr_t x, expr_t *e, interpreter_t *ctx);
expr_t f_vector(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_list_primitives(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_vector_ref(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_vector_set(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_vector_length(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_unquote(expr_t t, expr_t *e, interpreter_t *ctx);
expr_t f_quasiquote(expr_t t, expr_t *e, interpreter_t *ctx);