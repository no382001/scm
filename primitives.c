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
  if (_not(cdr(t))) {  // no second arg
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

L f_int(L t, L *e) {
  L n = car(evlis(t, *e));
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}

L f_lt(L t, L *e) { return t = evlis(t, *e), car(t) - car(cdr(t)) < 0 ? tru : nil; }

L f_eq(L t, L *e) { return t = evlis(t, *e), equ(car(t), car(cdr(t))) ? tru : nil; }

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

/* allows for local recursion where the name may also appear in the value of a letrec* name-value pair. 
   like this: (letrec* (f (lambda (n) (if (< 1 n) (* n (f (- n 1))) 1))) (f 5))
*/
L f_letreca(L t,L *e) { // this also should only accept lambdas
  for (; let(t); t = cdr(t)) {
    *e = pair(car(car(t)),err,*e);
    cell[sp+2] = eval(car(cdr(car(t))),*e);
  }
  return eval(car(t),*e);
}


/* evaluates all expressions first before binding the values */
L f_let(L t,L *e) {
  L d = *e;
  for (; let(t); t = cdr(t)) d = pair(car(car(t)),eval(car(cdr(car(t))),*e),d);
    return eval(car(t),d);
}


L f_lambda(L t, L *e) { return closure(car(t), car(cdr(t)), *e); }

L f_define(L t, L *e) {
  define_underway++;
  L res = eval(car(cdr(t)), *e);
  define_underway--;

  if (equ(res,err)){
    g_err_state.type = FUNCTION_DEF_IS_NOT_LAMBDA;
    g_err_state.box = car(t);
    g_err_state.proc = t;
    return res;
  }
  env = pair(car(t), res, env);
  //print(t);
  return car(t);
}


L f_macro(L t, L *e){
  return macro(car(t),car(cdr(t))); 
}

L f_load(L t, L *e) {
    L x = car(t);
    if (T(x) != ATOM) {
      g_err_state.type = LOAD_FILENAME_MUST_BE_ATOM;
      g_err_state.box = x;
      return err;
    }

    const char *filename = A + ord(x);
    FILE* file = fopen(filename, "r");
    if (!file) {
      g_err_state.type = LOAD_CANNOT_OPEN_FILE;
      g_err_state.box = x;
      return err;
    }
    parsing_ctx old_context = *curr_ctx;
    
    printf("--> loading %s...",filename);
    switch_ctx_to_file(file);

    L result = nil;
    while (curr_ctx->see != EOF) {
        L exp = Read();
        if (!equ(exp,err) && !equ(exp,nop)) {
            result = eval(exp, env);
            print(result); printf("\n");
            
        }
    }
    
    // close file and restore previous context
    fclose(curr_ctx->file);
    *curr_ctx = old_context;
    printf("--> loaded %s...",filename);
    
    return nop;
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
  return err;
}

L f_newline(L t, L *e) {
  if (!_not(t)){
    g_err_state.type = NEWLINE_TAKES_NO_ARG;
    g_err_state.box = t;
    return err;
  }
  putchar('\n');
  return err;
}

L f_begin(L t, L *e) {
  if (_not(t)){
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

L f_setq(L t,L *e) {
  L v = car(t);
  L x = eval(car(cdr(t)),*e);
  L env = *e;
  while (T(env) == CONS && !equ(v, car(car(env)))){
    env = cdr(env);
  }
  if (T(env) == CONS){
    cell[ord(car(env))] = x;
    return v;
  } else {
    g_err_state.type = SETQ_VAR_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}