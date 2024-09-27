#include "tinyscheme.h"

L f_eval(L t, L *e) { return car(evlis(t, *e)); }

L f_quote(L t, L *_) { return car(t); }

L f_cons(L t, L *e) { return t = evlis(t, *e), cons(car(t), car(cdr(t))); }

L f_car(L t, L *e) { return car(car(evlis(t, *e))); }

L f_cdr(L t, L *e) { return cdr(car(evlis(t, *e))); }

/* make a macro for prim proc also check for isnan*/
L f_add(L t, L *e) {
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n += car(t);
  return num(n);
}

L f_sub(L t, L *e) {
  L n = car(t = evlis(t, *e));
  if (_not(cdr(t))) { // no second arg
    return num(-n);
  }
  while (!_not(t = cdr(t)))
    n -= car(t);
  return num(n);
}

L f_mul(L t, L *e) {
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n *= car(t);
  return num(n);
}

L f_div(L t, L *e) {
  L n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t))) {
    L x = car(t);
    if (x == 0) {
      g_err_state.type = DIVIDE_ZERO;
      g_err_state.box = t;
      return err;
    }
    n /= x;
  }

  return num(n);
}

L f_int(L t, L *e) {
  L n = car(evlis(t, *e));
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}

L f_lt(L t, L *e) {
  return t = evlis(t, *e), car(t) - car(cdr(t)) < 0 ? tru : nil;
}

L f_eq(L t, L *e) {
  return t = evlis(t, *e), equ(car(t), car(cdr(t))) ? tru : nil;
}

L f_not(L t, L *e) { return _not(car(evlis(t, *e))) ? tru : nil; }

L f_or(L t, L *e) {
  L x = nil;
  while (T(t) != NIL && _not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}

L f_and(L t, L *e) {
  L x = nil;
  while (T(t) != NIL && !_not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}
L f_cond(L t, L *e) {
  while (T(t) != NIL && _not(eval(car(car(t)), *e)))
    t = cdr(t);
  return car(cdr(car(t)));
}
L f_if(L t, L *e) { return car(cdr(_not(eval(car(t), *e)) ? cdr(t) : t)); }

/* (let* (var1 expr1 ) (var2 expr2 ) ... (varn exprn ) expr) */
L f_leta(L t, L *e) {
  for (; let(t); t = cdr(t))
    *e = pair(car(car(t)), eval(car(cdr(car(t))), *e), *e);
  return car(t);
}

/* allows for local recursion where the name may also appear in the value of a
   letrec* name-value pair. like this: (letrec* (f (lambda (n) (if (< 1 n) (* n
   (f (- n 1))) 1))) (f 5))
*/
L f_letreca(L t, L *e) { // this also should only accept lambdas
  for (; let(t); t = cdr(t)) {
    *e = pair(car(car(t)), err, *e);
    cell[sp + 2] = eval(car(cdr(car(t))), *e);
  }
  return eval(car(t), *e);
}

/* evaluates all expressions first before binding the values */
L f_let(L t, L *e) {
  L d = *e;
  for (; let(t); t = cdr(t))
    d = pair(car(car(t)), eval(car(cdr(car(t))), *e), d);
  return eval(car(t), d);
}

L f_lambda(L t, L *e) { return closure(car(t), car(cdr(t)), *e); }

L f_define(L t, L *e) {
  define_underway++;
  L res = eval(car(cdr(t)), *e);
  define_underway--;

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

L f_macro(L t, L *e) { return macro(car(t), car(cdr(t))); }

L f_load(L t, L *e) {
  L x = eval(car(t), *e);
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

  // printf("--> loading %s...\n",filename);
  switch_ctx_to_file(file);

  L result = nil;
  while (curr_ctx->see != EOF) {
    L exp = Read();
    if (!equ(exp, err) && !equ(exp, nop)) {
      result = eval(exp, env);
      print(result);
      printf("\n");
    }
  }

  // close file and restore previous context
  fclose(curr_ctx->file);
  *curr_ctx = old_context;
  // printf("--> loaded %s...",filename);

  return nil;
}

L f_display(L t, L *e) {
  if (_not(t)) {
    g_err_state.type = DISPLAY_NO_ARG;
    return err;
  }

  L r = eval(car(t), *e);
  if (equ(r, err)) {
    g_err_state.type = DISPLAY_EVAL_ERROR; // maybe its a string? and print it?
    g_err_state.box = r;
    return err;
  }
  print(r);
  return nop; // might fuck up the include macro
}

L f_newline(L t, L *e) {
  if (!_not(t)) {
    g_err_state.type = NEWLINE_TAKES_NO_ARG;
    g_err_state.box = t;
    return err;
  }
  putchar('\n');
  return nop; // might fuck up the include macro
}

L f_begin(L t, L *e) {
  if (_not(t)) {
    g_err_state.type = BEGIN_NO_RETURN_VAL;
    g_err_state.box = t;
    return err;
  }
  L result = nil;
  while (T(t) == CONS) {
    result = eval(car(t), *e);
    t = cdr(t);
  }
  return result;
}

L f_setq(L t, L *e) {
  L v = car(t);
  L x = eval(car(cdr(t)), *e);
  L env = *e;
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

L f_trace(L x, L *e) {
  L t = car(x);
  L s = car(cdr(x));

  if (((int)num(t) != 0 && (int)num(t) != 1) ||
      ((int)num(s) != 0 && (int)num(s) != 1))
    return err;

  trace = (int)num(t);
  stepping = (int)num(s);

  longjmp(jb, 1);
}

L f_read(L t, L *e) {
  L x;

  char c = curr_ctx->see;
  curr_ctx->see = ' ';

  x = Read();
  curr_ctx->see = c;

  return x;
}

L f_gc(L x, L *e) {
  gc();
  return nop;
}

L f_rcrbcs(L x, L *e) {
  rcso_struct.x = x;
  rcso_struct.e = *e;
  trace_depth = 0;
  longjmp(jb, 2);
}

L f_setcar(L t, L *e) {
  L p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCAR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p) + 1] = car(cdr(t));
  return car(t);
}

L f_setcdr(L t, L *e) {
  L p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCDR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p)] = car(cdr(t));
  return car(t);
}

L f_atomq(L x, L *e) {
  L r = car(x);
  return T(r) == ATOM ? tru : nil;
}

L f_numberq(L x, L *e) {
  x = car(x);
  if (T(x) != NIL && T(x) != ATOM && T(x) != PRIM && T(x) != CONS &&
      T(x) != MACR && T(x) != NOP) {
    return tru;
  } else {
    return nil;
  }
}

L f_primq(L x, L *e) {
  L r = eval(car(x), *e);
  return T(r) == PRIM ? tru : nil;
}

L f_vector(L t, L *e) {
  L evaluated_list = evlis(t, *e);
  I size = 0;

  for (L tmp = evaluated_list; !_not(tmp); tmp = cdr(tmp)) {
    size++;
  }

  L vec = box(VECTOR, sp);
  cell[--sp] = size;

  for (I i = 0; i < size; ++i) {
    cell[--sp] = car(evaluated_list);
    evaluated_list = cdr(evaluated_list);
  }
  return vec;
}

L f_list_primitives(L t, L *e) {
  L result = nil;

  for (int i = 0; prim[i].s; ++i) {
    result = cons(atom(prim[i].s), result);
  }

  return result;
}

L f_vector_ref(L t, L *e) {
  L vec = car(evlis(t, *e));
  L idx = car(cdr(evlis(t, *e)));
  return vector_ref(vec, (I)num(idx));
}

L f_vector_set(L t, L *e) {
  L vec = car(evlis(t, *e));
  L idx = car(cdr(evlis(t, *e)));
  L val = car(cdr(cdr(evlis(t, *e))));
  return vector_set(vec, (I)num(idx), val);
}

L f_vector_length(L t, L *e) {
  L vec = car(evlis(t, *e));
  return vector_length(vec);
}
