#include "interpreter.h"
#include "print.h"

expr_t f_eval(expr_t t, expr_t *e) { return car(evlis(t, *e)); }

expr_t f_quote(expr_t t, expr_t *_) { return car(t); }

expr_t f_cons(expr_t t, expr_t *e) {
  return t = evlis(t, *e), cons(car(t), car(cdr(t)));
}

expr_t f_car(expr_t t, expr_t *e) { return car(car(evlis(t, *e))); }

expr_t f_cdr(expr_t t, expr_t *e) { return cdr(car(evlis(t, *e))); }

/* make a macro for prim proc also check for isnan*/
expr_t f_add(expr_t t, expr_t *e) {
  expr_t n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n += car(t);
  return num(n);
}

expr_t f_sub(expr_t t, expr_t *e) {
  expr_t n = car(t = evlis(t, *e));
  if (_not(cdr(t))) { // no second arg
    return num(-n);
  }
  while (!_not(t = cdr(t)))
    n -= car(t);
  return num(n);
}

expr_t f_mul(expr_t t, expr_t *e) {
  expr_t n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n *= car(t);
  return num(n);
}

expr_t f_div(expr_t t, expr_t *e) {
  expr_t n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t))) {
    expr_t x = car(t);
    if (x == 0) {
      g_err_state.type = DIVIDE_ZERO;
      g_err_state.box = t;
      return err;
    }
    n /= x;
  }

  return num(n);
}

expr_t f_int(expr_t t, expr_t *e) {
  expr_t n = car(evlis(t, *e));
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}

expr_t f_lt(expr_t t, expr_t *e) {
  return t = evlis(t, *e), car(t) - car(cdr(t)) < 0 ? tru : nil;
}

expr_t f_eq(expr_t t, expr_t *e) {
  return t = evlis(t, *e), equ(car(t), car(cdr(t))) ? tru : nil;
}

expr_t f_not(expr_t t, expr_t *e) {
  return _not(car(evlis(t, *e))) ? tru : nil;
}

expr_t f_or(expr_t t, expr_t *e) {
  expr_t x = nil;
  while (T(t) != NIL && _not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}

expr_t f_and(expr_t t, expr_t *e) {
  expr_t x = nil;
  while (T(t) != NIL && !_not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}

expr_t f_cond(expr_t t, expr_t *e) {
  while (T(t) != NIL && _not(eval(car(car(t)), *e)))
    t = cdr(t);
  return car(cdr(car(t)));
}

expr_t f_if(expr_t t, expr_t *e) {
  return car(cdr(_not(eval(car(t), *e)) ? cdr(t) : t));
}

/* (let* (var1 expr1 ) (var2 expr2 ) ... (varn exprn ) expr) */
expr_t f_leta(expr_t t, expr_t *e) {
  for (; let(t); t = cdr(t))
    *e = pair(car(car(t)), eval(car(cdr(car(t))), *e), *e);
  return car(t);
}

/* allows for local recursion where the name may also appear in the value of a
   letrec* name-value pair. like this: (letrec* (f (lambda (n) (if (< 1 n) (* n
   (f (- n 1))) 1))) (f 5))
*/
expr_t f_letreca(expr_t t, expr_t *e) { // this also should only accept lambdas
  for (; let(t); t = cdr(t)) {
    *e = pair(car(car(t)), err, *e);
    cell[sp + 2] = eval(car(cdr(car(t))), *e);
  }
  return eval(car(t), *e);
}

/* evaluates all expressions first before binding the values */
expr_t f_let(expr_t t, expr_t *e) {
  expr_t d = *e;
  for (; let(t); t = cdr(t))
    d = pair(car(car(t)), eval(car(cdr(car(t))), *e), d);
  return eval(car(t), d);
}

expr_t f_lambda(expr_t t, expr_t *e) {
  return closure(car(t), car(cdr(t)), *e);
}

expr_t f_define(expr_t t, expr_t *e) {
  if (!equ(assoc(car(t), env), err)) {
    printf("[warn] '%s' aliased in ", A + ord(car(t)));
    print(t);
    printf("\n");
  }
  g_err_state.type =
      NONE; // clear this, i should really refactor this whole eval thing
  g_err_state.box = nil;

  suppress_jumps++; // FIX: get rid of this shit
  expr_t res = eval(car(cdr(t)), *e);
  suppress_jumps--;

  if (equ(res, err)) {
    g_err_state.type = FUNCTION_DEF_IS_NOT_LAMBDA;
    g_err_state.box = car(t);
    g_err_state.proc = t;
    return res;
  }

  env = pair(car(t), res, env);
  // print(t);
  return car(t);
}

expr_t f_macro(expr_t t, expr_t *e) { return macro(car(t), car(cdr(t))); }

expr_t f_load(expr_t t, expr_t *e) {
  /*
  expr_t x = eval(car(t), *e);
  if (equ(x, err)) {
    g_err_state.type = LOAD_FILENAME_MUST_BE_QUOTED;
    g_err_state.box = t;
    return err;
  }

  const char *filename = A + ord(x);
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Failed to open file: errno = %d\n", errno);
    printf("Error message: %s\n", strerror(errno));
    g_err_state.type = LOAD_CANNOT_OPEN_FILE;
    g_err_state.box = x;
    return err;
  }
  parsing_ctx old_context = *curr_ctx;
  suppress_jumps++;
  // printf("--> loading %s...\n",filename);
  switch_ctx_to_file(file);

  // FIX: i dont like this at all why cant we do this in the main loop?
  // had some problems ctx switching and clearing faults before... so this will
  // do for now
  expr_t result = nil;
  while (curr_ctx->see != EOF) {
    print_and_reset_error();
    expr_t exp = Read();
    if (!equ(exp, err) && !equ(exp, nop)) {
      result = eval(exp, env);
      print(result);
      printf("\n");
    }
  }

  // close file and restore previous context
  fclose(curr_ctx->file);
  *curr_ctx = old_context;
  suppress_jumps--;
  */
  return nop;
}

expr_t f_display(expr_t t, expr_t *e) {
  if (_not(t)) {
    g_err_state.type = DISPLAY_NO_ARG;
    return err;
  }

  expr_t r = eval(car(t), *e);
  if (equ(r, err)) {
    g_err_state.type = DISPLAY_EVAL_ERROR; // maybe its a string? and print it?
    g_err_state.box = r;
    return err;
  }
  print(r);
  return nop; // might fuck up the include macro
}

expr_t f_newline(expr_t t, expr_t *e) {
  if (!_not(t)) {
    g_err_state.type = NEWLINE_TAKES_NO_ARG;
    g_err_state.box = t;
    return err;
  }
  putchar('\n');
  return nop; // might fuck up the include macro
}

expr_t f_begin(expr_t t, expr_t *e) {
  if (_not(t)) {
    g_err_state.type = BEGIN_NO_RETURN_VAL;
    g_err_state.box = t;
    return err;
  }
  expr_t result = nil;
  while (T(t) == CONS) {
    result = eval(car(t), *e);
    t = cdr(t);
  }
  return result;
}

expr_t f_setq(expr_t t, expr_t *e) {
  expr_t v = car(t);
  expr_t x = eval(car(cdr(t)), *e);
  expr_t env = *e;
  while (T(env) == CONS && !equ(v, car(car(env)))) {
    env = cdr(env);
  }
  if (T(env) == CONS) {
    cell[ord(car(env))] = x;
    return v;
  } else {
    g_err_state.type = SETQ_VAR_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}

expr_t f_trace(expr_t x, expr_t *e) {
  expr_t t = car(x);
  expr_t s = car(cdr(x));

  if (((int)num(t) != 0 && (int)num(t) != 1) ||
      ((int)num(s) != 0 && (int)num(s) != 1))
    return err;

  trace = (int)num(t);
  stepping = (int)num(s);

  longjmp(jb, 1);
}

expr_t f_read(expr_t t, expr_t *e) {
  /*
  expr_t x;
  char c = curr_ctx->see;
  curr_ctx->see = ' ';

  x = Read();
  curr_ctx->see = c;
  */
  return nil;
}

expr_t f_gc(expr_t x, expr_t *e) {
  gc();
  return nop;
}

expr_t f_rcrbcs(expr_t x, expr_t *e) {
  rcso_ctx.x = x;
  rcso_ctx.e = *e;
  trace_depth = 0;
  longjmp(jb, 2);
}

expr_t f_setcar(expr_t t, expr_t *e) {
  expr_t p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCAR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p) + 1] = car(cdr(t));
  return car(t);
}

expr_t f_setcdr(expr_t t, expr_t *e) {
  expr_t p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCDR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p)] = car(cdr(t));
  return car(t);
}

expr_t f_atomq(expr_t x, expr_t *e) {
  expr_t r = car(x);
  return T(r) == ATOM ? tru : nil;
}

expr_t f_numberq(expr_t x, expr_t *e) {
  x = eval(car(x), *e);
  if (T(x) != NIL && T(x) != ATOM && T(x) != PRIM && T(x) != CONS &&
      T(x) != MACR && T(x) != NOP) {
    return tru;
  } else {
    return nil;
  }
}

expr_t f_primq(expr_t x, expr_t *e) {
  expr_t r = eval(car(x), *e);
  return T(r) == PRIM ? tru : nil;
}

expr_t f_vector(expr_t t, expr_t *e) {
  expr_t evaluated_list = evlis(t, *e);
  tag_t size = 0;

  for (expr_t tmp = evaluated_list; !_not(tmp); tmp = cdr(tmp)) {
    size++;
  }

  expr_t vec = box(VECTOR, sp);
  cell[--sp] = size;

  for (tag_t i = 0; i < size; ++i) {
    cell[--sp] = car(evaluated_list);
    evaluated_list = cdr(evaluated_list);
  }
  return vec;
}

expr_t f_list_primitives(expr_t t, expr_t *e) {
  expr_t result = nil;

  for (int i = 0; prim[i].s; ++i) {
    result = cons(atom(prim[i].s), result);
  }

  return result;
}

expr_t f_vector_ref(expr_t t, expr_t *e) {
  expr_t vec = car(evlis(t, *e));
  expr_t idx = car(cdr(evlis(t, *e)));
  return vector_ref(vec, (tag_t)num(idx));
}

expr_t f_vector_set(expr_t t, expr_t *e) {
  expr_t vec = car(evlis(t, *e));
  expr_t idx = car(cdr(evlis(t, *e)));
  expr_t val = car(cdr(cdr(evlis(t, *e))));
  return vector_set(vec, (tag_t)num(idx), val);
}

expr_t f_vector_length(expr_t t, expr_t *e) {
  expr_t vec = car(evlis(t, *e));
  return vector_length(vec);
}

expr_t f_unquote(expr_t t, expr_t *e) {
  g_err_state.type = UNQUOTE_OUTSIDE_QUASIQUOTE;
  return err;
}

expr_t f_quasiquote(expr_t t, expr_t *e) { return eval_quasiquote(car(t), 1); }

expr_t eval_quasiquote(expr_t x, int level) {
  if (T(x) != CONS) {
    return x;
  }

  expr_t head = car(x);
  expr_t tail = cdr(x);

  if (T(head) == CONS && equ(car(head), atom("unquote"))) {
    if (level == 1) {
      expr_t result = eval(car(cdr(head)), env);
      return cons(result, eval_quasiquote(tail, level));
    } else {
      return cons(head, eval_quasiquote(tail, level));
    }
  }

  if (T(head) == CONS && equ(car(head), atom("quasiquote"))) {
    return cons(eval_quasiquote(head, level + 1), eval_quasiquote(tail, level));
  }

  return cons(eval_quasiquote(head, level), eval_quasiquote(tail, level));
}
