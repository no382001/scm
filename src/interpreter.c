#include "interpreter.h"

#define sp llc_sp
#define hp llc_hp
#define g_err_state llc_errstate
#define jb llc_jb
#define cell llc_cell

#define nil llc_c_nil
#define err llc_c_err
#define tru llc_c_tru
#define nil llc_c_nil

// defined in another section

// #define cons(exp1, exp2) cons(exp1, exp2, ctx)
// #define car(expr) car(expr, ctx)
// #define cdr(expr) cdr(expr, ctx)
// #define pair(v, x, e) pair(v, x, e, ctx)
// #define bind(v, t, e) bind(v, t, e, ctx)

expr_t box(tag_t t, tag_t i) {
  expr_t x;
  *(unsigned long long *)&x = (unsigned long long)t << 48 | i;
  return x;
}

tag_t ord(expr_t x) { return *(unsigned long long *)&x; }

expr_t num(expr_t n) { return n; }

tag_t equ(expr_t x, expr_t y) {
  return *(unsigned long long *)&x == *(unsigned long long *)&y;
}

/* --------------------------------------------- */
/*    low_level_ctx_t functions                  */
/* --------------------------------------------- */

expr_t cons(expr_t x, expr_t y, low_level_ctx_t *ctx) {
  cell[--sp] = x; /* push car of x  */
  cell[--sp] = y; /* push cdr of y */
  if (hp > sp << 3) {
    g_err_state.type = CONS_STACK_HEAP_COLLISION;
    longjmp(jb, 1);
  }
  return box(CONS, sp);
}

#define cons(exp1, exp2) cons(exp1, exp2, ctx)

expr_t car(expr_t p, low_level_ctx_t *ctx) {
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p) + 1];
  } else {
    g_err_state.type = CAR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

#define car(expr) car(expr, ctx)

expr_t cdr(expr_t p, low_level_ctx_t *ctx) {
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p)];
  } else {
    g_err_state.type = CDR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

#define cdr(expr) cdr(expr, ctx)

expr_t pair(expr_t v, expr_t x, expr_t e, low_level_ctx_t *ctx) {
  return cons(cons(v, x), e);
}

#define pair(v, x, e) pair(v, x, e, ctx)

expr_t bind(expr_t v, expr_t t, expr_t e, low_level_ctx_t *ctx) {
#define bind(v, t, e) bind(v, t, e, ctx)

  return T(v) == NIL    ? e
         : T(v) == CONS ? bind(cdr(v), cdr(t), pair(car(v), car(t), e))
                        : pair(v, t, e);
}

/* ------------------------- */

expr_t atom(const char *s, low_level_ctx_t *ctx) {
  tag_t i = 0;
  while (i < hp && strcmp(llc_atomheap + i, s)) /* search matching atom name */
    i += strlen(llc_atomheap + i) + 1;
  if (i == hp && (hp += strlen(strcpy(llc_atomheap + i, s)) + 1) >
                     sp << 3) { /* if not found, allocate and add a new atom
                                   name, abort when oom */
    g_err_state.type = ATOM_STACK_HEAP_COLLISION;
    longjmp(jb, 1);
  }
  return box(ATOM, i);
}

expr_t macro(expr_t v, expr_t x, low_level_ctx_t *ctx) {
  return box(MACR, ord(cons(v, x)));
}

/*
cell[20291] = 1
cell[20292] = 1
cell[20293] = 2
cell[20294] = ()
-> (vector 1 1)
*/
expr_t vector(tag_t size, low_level_ctx_t *ctx) {
  expr_t vec = box(VECTOR, sp);
  cell[--sp] = size;
  for (tag_t i = 0; i < size; ++i) {
    cell[--sp] = nil;
  }
  return vec;
}

expr_t vector_ref(expr_t vec, tag_t index, low_level_ctx_t *ctx) {
  if (T(vec) != VECTOR) {
    g_err_state.type = VECTOR_FN_NOT_A_VECTOR;
    g_err_state.box = vec;
    return err;
  }

  tag_t start = ord(vec);
  tag_t size = cell[start - 1];

  if (index >= size || index < 0) {
    g_err_state.type = VECTOR_FN_INDEX_OOB;
    return err;
  }

  return cell[start - 1 - index - 1];
}

expr_t vector_set(expr_t vec, tag_t index, expr_t value, low_level_ctx_t *ctx) {
  if (T(vec) != VECTOR) {
    g_err_state.type = VECTOR_FN_NOT_A_VECTOR;
    g_err_state.box = vec;
    return err;
  }

  tag_t start = ord(vec);
  tag_t size = cell[start - 1];

  if (index >= size || index < 0) {
    g_err_state.type = VECTOR_FN_INDEX_OOB;
    return err;
  }

  cell[start - 1 - index - 1] = value;

  return vec;
}

expr_t vector_length(expr_t vec, low_level_ctx_t *ctx) {
  if (T(vec) != VECTOR) {
    g_err_state.type = VECTOR_FN_NOT_A_VECTOR;
    g_err_state.box = vec;
    return err;
  }

  return num(cell[ord(vec) - 1]);
}

expr_t assoc(expr_t v, expr_t e, low_level_ctx_t *ctx) {
  while (T(e) == CONS && !equ(v, car(car(e)))) {
    e = cdr(e);
  }
  if (T(e) == CONS) {
    return cdr(car(e));
  } else {
    g_err_state.type = ASSOC_VALUE_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}

tag_t _not(expr_t x) { return T(x) == NIL; }

tag_t let(expr_t x, low_level_ctx_t *ctx) {
  return T(x) != NIL && (x = cdr(x), T(x) != NIL);
}

/* --------------------------------------------- */
/*    interpreter_t functions                    */
/* --------------------------------------------- */
#undef g_err_state
#undef nil
#undef cell
#undef sp
#undef hp
#undef err
#undef jb
#undef cons
#undef pair
#undef cdr
#undef cons
#undef car
#undef bind

#define g_err_state ic_errstate
#define env ic_env
#define nil ic_c_nil
#define cell ic_cell
#define err ic_c_err
#define sp ic_sp
#define hp ic_hp
#define jb ic_jb
#define pair(v, x, e) pair(v, x, e, ic2llcref)
#define cons(exp1, exp2) cons(exp1, exp2, ic2llcref)
#define assoc(v, e) assoc(v, e, ic2llcref)
#define step(expr, env) step(expr, env, ctx)
#define print(p) print(p, ctx);
#define cdr(expr) cdr(expr, ic2llcref)
#define cons(exp1, exp2) cons(exp1, exp2, ic2llcref)
#define car(expr) car(expr, ic2llcref)
#define bind(v, t, e) bind(v, t, e, ic2llcref)

expr_t eval(expr_t x, expr_t e, interpreter_t *ctx) {
#define eval(expr, env) eval(expr, env, ctx)
  expr_t in = x;
  ic_trace.trace_depth++;
  expr_t result = step(x, e);
  if (ic_trace.trace) {
    printf("[TRACE] %d\t", ic_trace.trace_depth);
    print(in);
    printf(" --> ");
    print(result);
    if (ic_trace.stepping) {
      while (getchar() >= ' ')
        continue;
    } else {
      putchar('\n');
    }
  }
  ic_trace.trace_depth--;

  return result;
}

// take this out of here
expr_t closure(expr_t v, expr_t x, expr_t e, interpreter_t *ctx) {
  return box(CLOS, ord(pair(v, x, equ(e, env) ? nil : e)));
}

// take this out of here
expr_t evlis(expr_t t, expr_t e, interpreter_t *ctx) {
#define evlis(expr, env) evlis(expr, env, ctx)

  expr_t s, *p;
  for (s = nil, p = &s; T(t) == CONS; p = cell + sp, t = cdr(t))
    *p = cons(eval(car(t), e), nil);
  if (T(t) == ATOM)
    *p = assoc(t, e);
  return s;
}

/* find the max heap reference among the used ATOM-tagged cells and adjust hp
 * accordingly */
void gc(interpreter_t *ctx) {
  tag_t i = sp = ord(env);
  for (hp = 0; i < N; ++i)
    if (T(cell[i]) == ATOM && ord(cell[i]) > hp)
      hp = ord(cell[i]);
  hp += strlen(ic_atomheap + hp) + 1;
}

// take this out of here
expr_t expand(expr_t f, expr_t t, expr_t e, interpreter_t *ctx) {
#define expand(f, t, e) expand(f, t, e, ctx)

  expr_t bind_r = bind(car(f), t, env);
  if (equ(bind_r, err)) {
    g_err_state.type = MACRO_EXPAND_BIND_FAILED;
    g_err_state.box = cdr(f);
    return err;
  }

  expr_t eval1_r = eval(cdr(f), bind_r);
  if (equ(eval1_r, err)) {
    g_err_state.type = MACRO_EXPAND_BODY_EVAL_FAILED;
    g_err_state.box = cdr(f);
    return err;
  }

  expr_t eval2_r = eval(eval1_r, e);
  if (equ(eval2_r, err)) {
    g_err_state.type = MACRO_EXPAND_EXECUTION_FAILED;
    g_err_state.box = eval1_r;
  }

  return eval2_r;
}

#undef step
expr_t step(expr_t x, expr_t e, interpreter_t *ctx) {

  expr_t f, v, d;
  while (1) {
    if (T(x) == ATOM) {
      return assoc(x, e);
    }
    if (T(x) != CONS) { /* could be a prim, return directly */
      return x;
    }

    expr_t proc = x;     /* save the proc for error message */
    f = eval(car(x), e); /* get proc sig */

    if (T(f) == MACR) {
      expr_t macro_args = cdr(x);
      x = expand(f, macro_args, e);
      return x;
    }
    /* it can fail, like in the case of cons?? or equ() is just for numbers? */

    x = cdr(x); /* get proc body */
    if (T(f) == PRIM) {
      x = ic_prim[ord(f)].f(x, &e, ctx); /* exec prim func */

      if (g_err_state.type) {
        if (!g_err_state.proc) {
          g_err_state.proc = proc;
        }
        if (!ic_jumps.suppress_jumps) { // i could just check for errtype inside
                                        // define
                                        // instead of this
          longjmp(jb, 1);
        }
      }

      if (ic_prim[ord(f)].t) /* do tc if its on */
        continue;
      return x;
    }

    if (T(f) != CLOS && T(f) != MACR) {
      g_err_state.type = EVAL_F_IS_NOT_A_FUNC;
      g_err_state.box = car(proc);
      g_err_state.proc = proc;
      return err;
    }

    v = car(car(f));   /* list of params from clos */
    d = cdr(f);        /* clos env */
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

#include "../util/enum_map.h"

int print_and_reset_error(interpreter_t *ctx) {
  if (g_err_state.type) {
    if (!ctx->noprint) {
      printf("%u: ", sp);
      printf("|%s| ", error_code_t_to_string[g_err_state.type]);
      print(g_err_state.box);
      printf(" @ ");
      print(g_err_state.proc);
      putchar('\n');
    }
    g_err_state.type = NONE;
    g_err_state.box = 0;
    g_err_state.proc = 0;
    return 1;
  }
  return 0;
}

prim_procs_t prim[] = {{"eval", f_eval, 1},
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
                       {"vector", f_vector, 0},
                       {"vector-ref", f_vector_ref, 0},
                       {"vector-set!", f_vector_set, 0},
                       {"vector-length", f_vector_length, 0},
                       {"__prims", f_list_primitives, 0},
                       {"unquote", f_unquote, 0},
                       {"quasiquote", f_quasiquote, 0},
                       {"__get-env", f_env, 0},
                       {0}};