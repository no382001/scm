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

L list() {
  L t = nil, *p = &t;
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
      get();        // eat dot
      *p = parse(); // parse the expression after the dot
      if (look().t != t_RPAREN) {
        return err;
      }
      get(); // eat the ')'
      return t;
    }

    *p = cons(parse(), nil);
    p = &cell[sp--]; // move to the next cell in the list
  }
}

L parse() {
  prim_t p = look();

  switch (p.t) {
  case t_TRUE:
    get();
    return tru;
  case t_FALSE:
    get();
    return nil;
  case t_LPAREN:
    return list();
  case t_QUOTE:
    get();
    return cons(atom("quote"), cons(parse(), nil));
  case t_QUASIQUOTE:
    get();
    return cons(atom("quasiquote"), cons(parse(), nil));
  case t_UNQUOTE:
    get();
    return cons(atom("unquote"), cons(parse(), nil));
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
