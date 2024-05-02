#include "tinyscheme.h"
#include "parser.c"
#include "primitives.c"
#include "print.c"

L box(I t, I i) {
  L x;
  *(unsigned long long *)&x = (unsigned long long)t << 48 | i;
  return x;
}

I ord(L x) { return *(unsigned long long *)&x; }

L num(L n) { return n; }

I equ(L x, L y) { return *(unsigned long long *)&x == *(unsigned long long *)&y; }

L atom(const char *s) {
  I i = 0;
  while (i < hp && strcmp(A + i, s)) /* search matching atom name */
    i += strlen(A + i) + 1;
  if (i == hp && (hp += strlen(strcpy(A + i, s)) + 1) > sp << 3) { /* if not found, allocate and add a new atom name, abort when oom */
    g_err_state.type = STACK_HEAP_COLLISION;
    longjmp(jb,1);
  } 
  return box(ATOM, i);
}

L macro(L v,L x) { return box(MACR,ord(cons(v,x))); }

L cons(L x, L y) {
  cell[--sp] = x; /* push car of x  */
  cell[--sp] = y; /* push cdr of y */
  if (hp > sp << 3){
    g_err_state.type = STACK_HEAP_COLLISION;
    longjmp(jb,1);
  }
  return box(CONS, sp);
}

L car(L p) { 
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p) + 1];
  } else {
    g_err_state.type = CAR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

L cdr(L p) {
  if (T(p) == CONS || T(p) == CLOS || T(p) == MACR) {
    return cell[ord(p)];
  } else {
    g_err_state.type = CDR_NOT_A_PAIR;
    g_err_state.box = p;
    return err;
  }
}

L pair(L v, L x, L e) { return cons(cons(v, x), e); }

L closure(L v, L x, L e) { return box(CLOS, ord(pair(v, x, equ(e, env) ? nil : e))); }

L assoc(L v, L e) {
  while (T(e) == CONS && !equ(v, car(car(e)))){
    e = cdr(e);
  }
  if (T(e) == CONS){
    return cdr(car(e));
  } else {
    g_err_state.type = ASSOC_VALUE_N_FOUND;
    g_err_state.box = v;
    return err;
  }
}

I _not(L x) { return T(x) == NIL; }

I let(L x) { return T(x) != NIL && (x = cdr(x), T(x) != NIL); }

L evlis(L t, L e) {
  L s, *p;
  for (s = nil, p = &s; T(t) == CONS; p = cell + sp, t = cdr(t))
    *p = cons(eval(car(t), e), nil);
  if (T(t) == ATOM)
    *p = assoc(t, e);
  return s;
}

L bind(L v,L t,L e) { return T(v) == NIL ? e : T(v) == CONS ?
  bind(
    cdr(v),cdr(t),pair(
      car(v),car(t),e)) : pair(v,t,e); }

L expand(L f,L t,L e) {
  L bind_r = bind(car(f),t,env);
  if (equ(bind_r,err)){
    g_err_state.type = MACRO_EXPAND_BIND_FAILED;
    g_err_state.box = cdr(f);
    return err;
  }
  
  L eval1_r = eval(cdr(f),bind_r);
  if (equ(eval1_r,err)){
    g_err_state.type = MACRO_EXPAND_BODY_EVAL_FAILED;
    g_err_state.box = cdr(f);
    return err;
  }
  
  L eval2_r = eval(eval1_r,e);
  if (equ(eval2_r,err)){
    g_err_state.type = MACRO_EXPAND_EXECUTION_FAILED;
    g_err_state.box = eval1_r;
  }

  return eval2_r;
}

L eval(L x, L e) {
  L in = x;
  trace_depth++;
  L result = step(x,e);
  print_heap();
  print_stack();
  if (trace){
    printf("[TRACE] %d\t",trace_depth); print(in); printf(" --> "); print(result);
    if (stepping) {
      while (getchar() >= ' ')
        continue;
    } else {
      putchar('\n');
    }
  }
  trace_depth--;
  print_heap();
  print_stack();

  return result;
}

L step(L x, L e) {

  L f, v, d;
  while (1) {
    if (T(x) == ATOM) {
      return assoc(x, e);
    }
    if (T(x) != CONS) { /* could be a prim, return directly */
      return x;
    }

    L proc = x; /* save the proc for error message */
    f = eval(car(x), e); /* get proc sig */

    if (T(f) == MACR) {
      L macro_args = cdr(x);
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
        if(!define_underway) { // i could just check for errtype inside define instead of this
          longjmp(jb,1);
        }
      }

      if (prim[ord(f)].t) /* do tc if its on */
        continue;
      return x;
    }
    
    if (T(f) != CLOS && T(f) != MACR){
      g_err_state.type = EVAL_F_IS_NOT_A_FUNC;
      g_err_state.box = car(proc);
      g_err_state.proc = proc;
      return err;
    }

    v = car(car(f)); /* list of params from clos */
    d = cdr(f); /* clos env */
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

/* find the max heap reference among the used ATOM-tagged cells and adjust hp accordingly */
void gc() {
  I i = sp = ord(env);
  for (hp = 0; i < N; ++i) if (T(cell[i]) == ATOM && ord(cell[i]) > hp) hp = ord(cell[i]);
    hp += strlen(A+hp)+1;
}

int print_and_reset_error() {
  if (g_err_state.type) {
    printf("%u: ",sp);
    printf("|%s| ",ERROR_T_to_string[g_err_state.type]);
    print(g_err_state.box); printf(" @ "); print(g_err_state.proc);  //putchar('\n');
    g_err_state.type = NONE;
    g_err_state.box = 0;
    g_err_state.proc = 0;
    return 1;
  }
  return 0;
}

#ifndef FUNC_TEST

int main() {
  original_stdin = dup(STDIN_FILENO);
  default_ctx.file = stdin;

  int i;
  nil = box(NIL, 0);
  err = atom("ERR"); // display and load returns this too but does not set an error status
  nop = box(NOP, 0);
  tru = atom("#t");
  env = pair(tru, tru, nil);
  for (i = 0; prim[i].s; ++i){
    env = pair(atom(prim[i].s), box(PRIM, i), env);
  }

  printf("  __   .__                                        ._.\n");
  printf("_/  |_ |__|  ____  ___.__.  ______  ____    _____ | |\n");
  printf("\\   __\\|  | /    \\<   |  | /  ___/_/ ___\\  /     \\| |\n");
  printf(" |  |  |  ||   |  \\\\___  | \\___ \\ \\  \\___ |  Y Y  \\\\|\n");
  printf(" |__|  |__||___|  // ____|/____  > \\___  >|__|_|  /__\n");
  printf("                \\/ \\/          \\/      \\/       \\/ \\/\n");

  setjmp(jb);

  while (1) {
    print_and_reset_error();
    gc();
    printf("\n%u>", sp - hp / 8);
    L res = eval(Read(), env);
    if (!equ(err,res) && !equ(err,nop)){
      print(res);
    }
  }
}

#endif