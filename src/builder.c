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
  while (1) {
    prim_t token = next(); // eat '('
    if (token.t == t_RPAREN || token.t == t_END_OF_FILE) {
      get();
      return t;
    }

    // handle dotted pairs
    if (token.t == t_DOT) {
      get();         // eat dot
      *p = parse();  // thing after the dot
      token = get(); // eat token after thing (should be ')')
      if (token.t != t_RPAREN) {
        return err;
      }
      return t;
    }

    // next
    *p = cons(parse(), nil);
    p = &cell[sp--]; // move to the next cell in the list
  }
}

L parse() {
  prim_t p = look();
  switch (p.t) {
  case t_TRUE:
    return tru;
  case t_FALSE:
    return nil;
  case t_LPAREN:
    return list();
  case t_QUOTE:
    return cons(atom("quote"), cons(parse(), nil));
  case t_QUASIQUOTE:
    return cons(atom("quasiquote"), cons(parse(), nil));
  case t_UNQUOTE:
    return cons(atom("unquote"), cons(parse(), nil));
  case t_ATOM:
    return atom(p.str);
  case t_NUMBER:
    return num(p.num);
  case t_NEWLINE:
  case t_COMMENT:
    return nop;
  default:
    return err;
    break;
  }
}
