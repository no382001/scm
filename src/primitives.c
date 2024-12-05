#include "interpreter.h"
#include "print.h"

#define car(expr) car(expr, ic2llcref)
#define cdr(expr) cdr(expr, ic2llcref)
#define cons(exp1, exp2) cons(exp1, exp2, ic2llcref)
#define evlis(expr, env) evlis(expr, env, ctx)
#define eval(expr, env) eval(expr, env, ctx)
#define let(expr) let(expr, ic2llcref)
#define pair(v, x, e) pair(v, x, e, ic2llcref)
#define closure(v, x, e) closure(v, x, e, ctx);
#define assoc(v, e) assoc(v, e, ic2llcref)
#define print(x) print(x, ctx)
#define macro(v, x) macro(v, x, ic2llcref)
#define atom(s) atom(s, ic2llcref)

#define g_err_state ic_errstate
#define err ic_c_err
#define tru ic_c_tru
#define nil ic_c_nil
#define nop ic_c_nop
#define cell ic_cell
#define sp ic_sp
#define env ic_env
#define jb ic_jumps.jb
#define rcso_ctx ic_rcso

expr_t f_eval(expr_t t, expr_t *e, interpreter_t *ctx) {
  int count = 0;
  expr_t temp = t;
  while (T(temp) == CONS) {
    count++;
    temp = cdr(temp);
  }
  if (count == 1) {
    return car(evlis(t, *e));
  } else if (count == 2) {
    expr_t x = eval(car(t), *e);
    expr_t curr_env = eval(car(cdr(t)), *e);
    return car(evlis(x, curr_env));
  } else {
    ic_errstate.type = EVAL_WRONG_N_OF_ARGS;
    return err;
  }
}

expr_t f_env(expr_t t, expr_t *e, interpreter_t *ctx) { return *e; }

expr_t f_quote(expr_t t, expr_t *_, interpreter_t *ctx) { return car(t); }

expr_t f_cons(expr_t t, expr_t *e, interpreter_t *ctx) {
  return t = evlis(t, *e), cons(car(t), car(cdr(t)));
}

expr_t f_car(expr_t t, expr_t *e, interpreter_t *ctx) {
  return car(car(evlis(t, *e)));
}

expr_t f_cdr(expr_t t, expr_t *e, interpreter_t *ctx) {
  return cdr(car(evlis(t, *e)));
}

/* make a macro for prim proc also check for isnan*/
expr_t f_add(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n += car(t);
  return num(n);
}

expr_t f_sub(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t n = car(t = evlis(t, *e));
  if (_not(cdr(t))) { // no second arg
    return num(-n);
  }
  while (!_not(t = cdr(t)))
    n -= car(t);
  return num(n);
}

expr_t f_mul(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t n = car(t = evlis(t, *e));
  while (!_not(t = cdr(t)))
    n *= car(t);
  return num(n);
}

expr_t f_div(expr_t t, expr_t *e, interpreter_t *ctx) {
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

expr_t f_int(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t n = car(evlis(t, *e));
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}

expr_t f_lt(expr_t t, expr_t *e, interpreter_t *ctx) {
  return t = evlis(t, *e), car(t) - car(cdr(t)) < 0 ? tru : nil;
}

expr_t f_eq(expr_t t, expr_t *e, interpreter_t *ctx) {
  return t = evlis(t, *e), equ(car(t), car(cdr(t))) ? tru : nil;
}

expr_t f_not(expr_t t, expr_t *e, interpreter_t *ctx) {
  return _not(car(evlis(t, *e))) ? tru : nil;
}

expr_t f_or(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t x = nil;
  while (T(t) != NIL && _not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}

expr_t f_and(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t x = nil;
  while (T(t) != NIL && !_not(x = eval(car(t), *e)))
    t = cdr(t);
  return x;
}

expr_t f_cond(expr_t t, expr_t *e, interpreter_t *ctx) {
  while (T(t) != NIL && _not(eval(car(car(t)), *e)))
    t = cdr(t);
  return car(cdr(car(t)));
}

expr_t f_if(expr_t t, expr_t *e, interpreter_t *ctx) {
  return car(cdr(_not(eval(car(t), *e)) ? cdr(t) : t));
}

/* (let* (var1 expr1 ) (var2 expr2 ) ... (varn exprn ) expr) */
expr_t f_leta(expr_t t, expr_t *e, interpreter_t *ctx) {
  for (; let(t); t = cdr(t))
    *e = pair(car(car(t)), eval(car(cdr(car(t))), *e), *e);
  return car(t);
}

/* allows for local recursion where the name may also appear in the value of a
   letrec* name-value pair. like this: (letrec* (f (lambda (n) (if (< 1 n) (* n
   (f (- n 1))) 1))) (f 5))
*/
expr_t f_letreca(expr_t t, expr_t *e,
                 interpreter_t *ctx) { // this also should only accept lambdas
  for (; let(t); t = cdr(t)) {
    *e = pair(car(car(t)), err, *e);
    cell[sp + 2] = eval(car(cdr(car(t))), *e);
  }
  return eval(car(t), *e);
}

/* evaluates all expressions first before binding the values */
expr_t f_let(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t d = *e;
  for (; let(t); t = cdr(t))
    d = pair(car(car(t)), eval(car(cdr(car(t))), *e), d);
  return eval(car(t), d);
}

expr_t f_lambda(expr_t t, expr_t *e, interpreter_t *ctx) {
  return closure(car(t), car(cdr(t)), *e);
}

expr_t f_define(expr_t t, expr_t *e, interpreter_t *ctx) {
  if (!equ(assoc(car(t), env), err)) {
    printf("[warn] '%s' aliased in ", ic_atomheap + ord(car(t)));
    print(t);
    printf("\n");
  }
  g_err_state.type =
      NONE; // clear this, i should really refactor this whole eval thing
  g_err_state.box = nil;

  ic_jumps.suppress_jumps++; // FIX: get rid of this shit
  expr_t res = eval(car(cdr(t)), *e);
  ic_jumps.suppress_jumps--;

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

expr_t f_macro(expr_t t, expr_t *e, interpreter_t *ctx) {
  return macro(car(t), car(cdr(t)));
}

#include "interpreter.h"
#include "parser.h"
extern parse_ctx *curr_ctx;
expr_t repl(read_ctx_t *rc);
int f_load_layer = -1;
expr_t f_load(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t x = eval(car(t), *e);
  if (T(x) != STRING) {
    g_err_state.type = LOAD_FILENAME_MUST_BE_A_STRING;
    g_err_state.box = t;
    return err;
  }

  const char *filename = ic_atomheap + ord(x);

  FILE *file = fopen(filename, "r");
  if (!file) {
    char find_command[1024];
    snprintf(find_command, sizeof(find_command), "find . -name '%s'", filename);

    FILE *find_output = popen(find_command, "r");
    if (!find_output) {
      printf("error executing find command.\n");
      g_err_state.type = LOAD_CANNOT_OPEN_FILE;
      g_err_state.box = x;
      return err;
    }

    char found_path[1024];
    if (fgets(found_path, sizeof(found_path), find_output)) {
      found_path[strcspn(found_path, "\n")] = 0;
      filename = found_path;
      file = fopen(filename, "r");
    }

    pclose(find_output);

    if (!file) {
      g_err_state.type = LOAD_CANNOT_OPEN_FILE;
      g_err_state.box = x;
      return err;
    }
  }

  parse_ctx *old_ctx = deep_copy_parse_ctx(curr_ctx);
  if (!old_ctx) {
    fclose(file);
    return err;
  }

  switch_ctx_to_file(file);

  token_buffer_t tb = {0};
  read_ctx_t rc = {
      .ic = ctx, .tb = &tb, .read = read_line, .f_load_layer = f_load_layer++};

  expr_t result = nil;

  result = repl(&rc); // does it ever exit?

  curr_ctx = old_ctx;
  ctx->nosetjmp = false;
  f_load_layer--;

  return result;
}

expr_t f_display(expr_t t, expr_t *e, interpreter_t *ctx) {
  if (_not(t)) {
    g_err_state.type = DISPLAY_NO_ARG;
    return err;
  }

  expr_t r = eval(car(t), *e);
  if (equ(r, err)) {
    g_err_state.type = DISPLAY_EVAL_ERROR;
    g_err_state.box = r;
    return err;
  }
  print(r);
  return nop;
}

expr_t f_newline(expr_t t, expr_t *e, interpreter_t *ctx) {
  if (!_not(t)) {
    g_err_state.type = NEWLINE_TAKES_NO_ARG;
    g_err_state.box = t;
    return err;
  }
  putchar('\n');
  return nop; // might fuck up the include macro
}

expr_t f_begin(expr_t t, expr_t *e, interpreter_t *ctx) {
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

expr_t f_setq(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t v = car(t);
  expr_t x = eval(car(cdr(t)), *e);
  expr_t _env = *e;
  while (T(_env) == CONS && !equ(v, car(car(_env)))) {
    _env = cdr(_env);
  }
  if (T(_env) == CONS) {
    cell[ord(car(_env))] = x;
    return v;
  } else {
    g_err_state.type = SETQ_VAR_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}

expr_t f_trace(expr_t x, expr_t *e, interpreter_t *ctx) {
  expr_t t = car(x);
  expr_t s = car(cdr(x));

  if (((int)num(t) != 0 && (int)num(t) != 1) ||
      ((int)num(s) != 0 && (int)num(s) != 1))
    return err;

  ic_trace.trace = (bool)num(t);
  ic_trace.stepping = (bool)num(s);

  longjmp(jb, 1);
}

expr_t f_read(expr_t t, expr_t *e, interpreter_t *ctx) {
  if (!_not(t)) {
    g_err_state.type = READ_TAKES_NO_ARG;
    g_err_state.box = t;
    return err;
  }

  int c = getchar();
  if (c == EOF) {
    g_err_state.type = READ_EOF;
    return err;
  }

  char b[2] = {c, '\0'};
  return atom(b);
}

expr_t f_gc(expr_t x, expr_t *e, interpreter_t *ctx) {
  compact_atom_heap(ctx);
  gc(ctx);
  return nil;
}

expr_t f_rcrbcs(expr_t x, expr_t *e, interpreter_t *ctx) {
  rcso_ctx.x = x;
  rcso_ctx.e = *e;
  ic_trace.trace_depth = 0;
  longjmp(jb, 2);
}

expr_t f_setcar(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCAR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p) + 1] = car(cdr(t));
  return car(t);
}

expr_t f_setcdr(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t p = car(t = evlis(t, *e));
  if (T(p) != CONS) {
    g_err_state.type = SETCDR_ARG_NOT_CONS;
    g_err_state.box = t;
    return err;
  }
  cell[ord(p)] = car(cdr(t));
  return car(t);
}

expr_t f_atomq(expr_t x, expr_t *e, interpreter_t *ctx) {
  expr_t r = car(x);
  return T(r) == ATOM ? tru : nil;
}

expr_t f_numberq(expr_t x, expr_t *e, interpreter_t *ctx) {
  x = eval(car(x), *e);
  if (T(x) != NIL && T(x) != ATOM && T(x) != PRIM && T(x) != CONS &&
      T(x) != MACR && T(x) != NOP) {
    return tru;
  } else {
    return nil;
  }
}

expr_t f_primq(expr_t x, expr_t *e, interpreter_t *ctx) {
  expr_t r = eval(car(x), *e);
  return T(r) == PRIM ? tru : nil;
}

expr_t f_vector(expr_t t, expr_t *e, interpreter_t *ctx) {
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

expr_t f_list_primitives(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t result = nil;

  for (int i = 0; ic_prim[i].s; ++i) {
    result = cons(atom(ic_prim[i].s), result);
  }

  return result;
}

expr_t f_vector_ref(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t vec = car(evlis(t, *e));
  expr_t idx = car(cdr(evlis(t, *e)));
  return vector_ref(vec, (tag_t)num(idx), ic2llcref);
}

expr_t f_vector_set(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t vec = car(evlis(t, *e));
  expr_t idx = car(cdr(evlis(t, *e)));
  expr_t val = car(cdr(cdr(evlis(t, *e))));
  return vector_set(vec, (tag_t)num(idx), val, ic2llcref);
}

expr_t f_vector_length(expr_t t, expr_t *e, interpreter_t *ctx) {
  expr_t vec = car(evlis(t, *e));
  return vector_length(vec, ic2llcref);
}

expr_t f_unquote(expr_t t, expr_t *e, interpreter_t *ctx) {
  g_err_state.type = UNQUOTE_OUTSIDE_QUASIQUOTE;
  return err;
}

static expr_t eval_quasiquote(expr_t x, expr_t e, int level,
                              interpreter_t *ctx) {
  if (T(x) != CONS) {
    return x;
  }

  expr_t head = car(x);
  expr_t tail = cdr(x);

  if (T(head) == CONS && equ(car(head), atom("unquote"))) {
    if (level == 1) {
      expr_t result = eval(car(cdr(head)), e);
      return cons(result, eval_quasiquote(tail, e, level, ctx));
    } else {
      return cons(head, eval_quasiquote(tail, e, level, ctx));
    }
  }

  if (T(head) == CONS && equ(car(head), atom("quasiquote"))) {
    return cons(eval_quasiquote(head, e, level + 1, ctx),
                eval_quasiquote(tail, e, level, ctx));
  }

  return cons(eval_quasiquote(head, e, level, ctx),
              eval_quasiquote(tail, e, level, ctx));
}

expr_t f_quasiquote(expr_t t, expr_t *e, interpreter_t *ctx) {
  return eval_quasiquote(car(t), *e, 1, ctx);
}

// (with-exception-handler (+ 1 n) (+ 1 1))
// top level statements are invalid
// (with-exception-handler n (+ 1 1)) this fails on ASSOC
expr_t f_weh(expr_t t, expr_t *e, interpreter_t *ctx) {
  jmp_buf tjb;
  memcpy(tjb, jb, sizeof(jmp_buf));

  int jmpres = setjmp(jb);
  expr_t res = nop;
  if (jmpres == 1) {
    // print_and_reset_error(ctx);
    if (g_err_state.type) {
      g_err_state.type = NONE;
      g_err_state.box = 0;
      g_err_state.proc = 0;
    }
    res = eval(car(cdr(t)), *e);
  } else if (jmpres == 2) {
    res = eval(ic_rcso.x, ic_rcso.e);
  } else {
    res = eval(car(t), *e);
  }

  memcpy(jb, tjb, sizeof(jmp_buf));
  return res;
}

#include <time.h>
expr_t f_time(expr_t t, expr_t *e, interpreter_t *ctx) {
    if (_not(t)) {
        g_err_state.type = TIME_NO_ARG;
        g_err_state.box = t;
        return err;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    expr_t result = eval(car(t), *e);

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    double elapsed_ms = seconds * 1000.0 + nanoseconds / 1000000.0;

    return num(elapsed_ms);
}