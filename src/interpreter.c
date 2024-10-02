#include "interpreter.h"

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

expr_t atom(const char *s) {
  tag_t i = 0;
  while (i < hp && strcmp(A + i, s)) /* search matching atom name */
    i += strlen(A + i) + 1;
  if (i == hp && (hp += strlen(strcpy(A + i, s)) + 1) >
                     sp << 3) { /* if not found, allocate and add a new atom
                                   name, abort when oom */
    g_err_state.type = STACK_HEAP_COLLISION;
    longjmp(jb, 1);
  }
  return box(ATOM, i);
}

expr_t macro(expr_t v, expr_t x) { return box(MACR, ord(cons(v, x))); }

expr_t cons(expr_t x, expr_t y) {
  cell[--sp] = x; /* push car of x  */
  cell[--sp] = y; /* push cdr of y */
  if (hp > sp << 3) {
    g_err_state.type = STACK_HEAP_COLLISION;
    longjmp(jb, 1);
  }
  return box(CONS, sp);
}

/*
cell[20291] = 1
cell[20292] = 1
cell[20293] = 2
cell[20294] = ()
-> (vector 1 1)
*/
expr_t vector(tag_t size) {
  expr_t vec = box(VECTOR, sp);
  cell[--sp] = size;
  for (tag_t i = 0; i < size; ++i) {
    cell[--sp] = nil;
  }
  return vec;
}

expr_t vector_ref(expr_t vec, tag_t index) {
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

expr_t vector_set(expr_t vec, tag_t index, expr_t value) {
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

expr_t vector_length(expr_t vec) {
  if (T(vec) != VECTOR) {
    g_err_state.type = VECTOR_FN_NOT_A_VECTOR;
    g_err_state.box = vec;
    return err;
  }

  return num(cell[ord(vec) - 1]);
}

expr_t car(expr_t p) {
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p) + 1];
  } else {
    g_err_state.type = CAR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

expr_t cdr(expr_t p) {
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p)];
  } else {
    g_err_state.type = CDR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

expr_t pair(expr_t v, expr_t x, expr_t e) { return cons(cons(v, x), e); }

expr_t closure(expr_t v, expr_t x, expr_t e) {
  return box(CLOS, ord(pair(v, x, equ(e, env) ? nil : e)));
}

expr_t assoc(expr_t v, expr_t e) {
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

tag_t let(expr_t x) { return T(x) != NIL && (x = cdr(x), T(x) != NIL); }

expr_t evlis(expr_t t, expr_t e) {
  expr_t s, *p;
  for (s = nil, p = &s; T(t) == CONS; p = cell + sp, t = cdr(t))
    *p = cons(eval(car(t), e), nil);
  if (T(t) == ATOM)
    *p = assoc(t, e);
  return s;
}

expr_t bind(expr_t v, expr_t t, expr_t e) {
  return T(v) == NIL    ? e
         : T(v) == CONS ? bind(cdr(v), cdr(t), pair(car(v), car(t), e))
                        : pair(v, t, e);
}

expr_t expand(expr_t f, expr_t t, expr_t e) {
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

void print(expr_t x);

expr_t eval(expr_t x, expr_t e) {
  expr_t in = x;
  trace_depth++;
  expr_t result = step(x, e);
  if (trace) {
    printf("[TRACE] %d\t", trace_depth);
    print(in);
    printf(" --> ");
    print(result);
    if (stepping) {
      while (getchar() >= ' ')
        continue;
    } else {
      putchar('\n');
    }
  }
  trace_depth--;

  return result;
}

expr_t step(expr_t x, expr_t e) {

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
      x = prim[ord(f)].f(x, &e); /* exec prim func */

      if (g_err_state.type) {
        if (!g_err_state.proc) {
          g_err_state.proc = proc;
        }
        if (!suppress_jumps) { // i could just check for errtype inside define
                               // instead of this
          longjmp(jb, 1);
        }
      }

      if (prim[ord(f)].t) /* do tc if its on */
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

/* find the max heap reference among the used ATOM-tagged cells and adjust hp
 * accordingly */
void gc() {
  tag_t i = sp = ord(env);
  for (hp = 0; i < N; ++i)
    if (T(cell[i]) == ATOM && ord(cell[i]) > hp)
      hp = ord(cell[i]);
  hp += strlen(A + hp) + 1;
}

int print_and_reset_error() {
  if (g_err_state.type) {
    printf("%u: ", sp);
    printf("|%s| ", ERROR_T_to_string[g_err_state.type]);
    print(g_err_state.box);
    printf(" @ ");
    print(g_err_state.proc); // putchar('\n');
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
                       {0}};