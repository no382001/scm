#include "tinyscheme.h"

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

    if (dup2(fileno(file), STDIN_FILENO) == -1) {
      fclose(file);
      g_err_state.type = LOAD_FAILED_TO_REDIRECT_STDIN;
      g_err_state.box = x;
      return err;
    }

    fclose(file);
    longjmp(jb,1);
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


L eval(L x, L e) {

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

void skip_multiline_comment() {
  get(); // consume the '|' after '#'
  while (1) {
    if (see == '|') {
      get(); // consume the '|'
      if (see == '#') {
        get(); // consume the '#', end of comment
        return;
      }
    } else {
      get(); // continue reading through the comment
    }
  }
}

void look() {
  int c = getchar();
  if (c == EOF) {
    // check if EOF is due to an actual end of file condition
    if (feof(stdin)) {
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);
        clearerr(stdin);  // clear EOF condition on stdin
        see = ' ';
        return;     // return to avoid setting see to EOF
      } else {
        exit(1);  // exit if EOF on the original stdin and not processing a file
    }
  }
  see = c;
}

I seeing(char c) { return c == ' ' ? see > 0 && see <= c : see == c; }

char get() {
  char c = see;
  look();
  return c;
}

char scan() {
  int i = 0;
  while (seeing(' ')){
    look();
  }
  // multiline comments
  if (see == '#') { // uhmm, this might have some consequences no?
    get();
    if (see == '|') {
      skip_multiline_comment();
      return scan();
    }
  // single line comment
  } else if (seeing(';')) {
    while (!seeing('\n') && see != EOF) {
      look();
    }
    return scan();
  } else if (seeing('(') || seeing(')') || seeing('\'')){
    char c = get();
    buf[i++] = c;
  } else {
    do {
      char c = get();
      buf[i++] = c;
    } while (i < PARSE_BUFFER-1 && !seeing('(') && !seeing(')') && !seeing(' '));
  }
  return buf[i] = 0, *buf;
}

L Read() {
  if (see == EOF){
    return nil;
  }
  return scan(), parse();
}

L list() {
  L t, *p;
  for (t = nil, p = &t;; *p = cons(parse(), nil), p = cell + sp)
  {
    if (scan() == ')')
      return t;
    if (*buf == '.' && !buf[1])
      return *p = Read(), scan(), t;
  }
}

L parse() {
  L n;
  int i;
  if (*buf == '(')
    return list();
  if (*buf == '\'')
    return cons(atom("quote"), cons(Read(), nil));
  return sscanf(buf, "%lg%n", &n, &i) > 0 && !buf[i] ? n : atom(buf);
}


void printlist(L t) {
  for (putchar('(');; putchar(' '))
  {
    print(car(t));
    if (_not(t = cdr(t)))
      break;
    if (T(t) != CONS)
    {
      printf(" . ");
      print(t);
      break;
    }
  }
  putchar(')');
}

void print(L x) {
  if (T(x) == NIL)
    printf("()");
  else if (T(x) == ATOM)
    printf("%s", A + ord(x));
  else if (T(x) == PRIM)
    printf("<%s>", prim[ord(x)].s);
  else if (T(x) == CONS || T(x) == MACR)
    printlist(x);
  else if (T(x) == CLOS)
    printf("{%u}", ord(x));
  else
    printf("%.10lg", x);
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

void print_stack() {
  printf("Stack contents:\n");
  for (I i = sp; i < N; i++) {
    printf("cell[%u] = ", i);
    print(cell[i]);
    printf("\n");
  }
}


void print_heap() {
  printf("Heap contents:\n");
  for (I i = 0; i < hp; i++) {
    printf("cell[%u] = ", i);
    print(cell[i]);
    printf("\n");
  }
}

#ifndef FUNC_TEST

int main() {
  original_stdin = dup(STDIN_FILENO);
  int i;
  nil = box(NIL, 0);
  err = atom("ERR"); // display and load returns this too but does not set an error status
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
    if (!equ(err,res)){
      print(res);
    }
  }
}

#endif