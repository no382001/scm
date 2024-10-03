#include "interpreter.h"
#include "parser.h"
#include "print.h"

int token_idx = 0;

// what are we looking at?
prim_t look() { return token_buffer[token_idx]; }

// gets the curr, and advances
prim_t get() { return token_buffer[token_idx++]; }

// advance and get
prim_t next() { return token_buffer[++token_idx]; }

// backwards and get
prim_t prev() { return token_buffer[--token_idx]; }

#define cons(exp1, exp2) cons(exp1, exp2, ic2llcref)
#define atom(s) atom(s, ic2llcref)
#define sp ic_sp
#define cell ic_cell
#define nil ic_c_nil
#define tru ic_c_tru
#define err ic_c_err
#define nop ic_c_nop

expr_t list(interpreter_t *ctx) {
  expr_t t = nil, *p = &t;
  get(); // eat the '('

  while (1) {
    prim_t token = look();

    if (token.t == t_RPAREN) {
      get(); // eat the ')'
      return t;
    }

    if (token.t == t_END_OF_FILE) {
      return err;
    }

    if (token.t == t_DOT) {
      get();           // eat dot
      *p = parse(ctx); // parse the expression after the dot
      if (look().t != t_RPAREN) {
        return err;
      }
      get(); // eat the ')'
      return t;
    }

    *p = cons(parse(ctx), nil);
    p = &cell[sp--]; // move to the next cell in the list
  }
}

expr_t parse(interpreter_t *ctx) {
  prim_t p = look();

  switch (p.t) {
  case t_TRUE:
    get();
    return tru;
  case t_FALSE:
    get();
    return nil;
  case t_LPAREN:
    return list(ctx);
  case t_QUOTE:
    get();
    return cons(atom("quote"), cons(parse(ctx), nil));
  case t_QUASIQUOTE:
    get();
    return cons(atom("quasiquote"), cons(parse(ctx), nil));
  case t_UNQUOTE:
    get();
    return cons(atom("unquote"), cons(parse(ctx), nil));
  case t_ATOM:
    get();
    return atom(p.str);
  case t_NUMBER:
    get();
    return num(p.num);
  case t_NEWLINE:
  case t_COMMENT:
    get();
    return nop;
  default:
    return err;
  }
}
