#include "interpreter.h"
#include "parser.h"
#include "print.h"

void reseti(token_buffer_t *tb) { tb->idx = 0; }

// what are we looking at?
prim_t look(token_buffer_t *tb) { return tb->buffer[tb->idx]; }

// gets the curr, and advances
prim_t get(token_buffer_t *tb) { return tb->buffer[tb->idx++]; }

// advance and get
prim_t next(token_buffer_t *tb) { return tb->buffer[++tb->idx]; }

// backwards and get
prim_t prev(token_buffer_t *tb) { return tb->buffer[--tb->idx]; }

#define cons(exp1, exp2) cons(exp1, exp2, ic2llcref)
#define atom(s) atom(s, ic2llcref)
#define sp ic_sp
#define cell ic_cell
#define nil ic_c_nil
#define tru ic_c_tru
#define err ic_c_err
#define nop ic_c_nop

#define get() get(tb)
#define look() look(tb)
#define list() list(ctx, tb)

expr_t static vector_conv(interpreter_t *ctx, token_buffer_t *tb) {
  expr_t inner = list();
  return cons(atom("vector"), inner);
}

expr_t parse(interpreter_t *ctx, token_buffer_t *tb) {
#define parse() parse(ctx, tb)

  prim_t p = look();

  switch (p.t) {
  case TAG_TRUE:
    get();
    return tru;
  case TAG_FALSE:
    get();
    return nil;
  case TAG_LPAREN:
    return list();
  case TAG_VECTOR:
    return vector_conv(ctx, tb);
  case TAG_QUOTE:
    get();
    return cons(atom("quote"), cons(parse(), nil));
  case TAG_QUASIQUOTE:
    get();
    return cons(atom("quasiquote"), cons(parse(), nil));
  case TAG_UNQUOTE:
    get();
    return cons(atom("unquote"), cons(parse(), nil));
  case TAG_ATOM:
    get();
    return atom(p.str);
  case TAG_NUMBER:
    get();
    return num(p.num);
  case TAG_NEWLINE:
  case TAG_COMMENT:
    get();
    return nop;
  default:
    return err;
  }
}

#undef list
expr_t list(interpreter_t *ctx, token_buffer_t *tb) {
  expr_t t = nil, *p = &t;
  get(); // eat the '('

  while (1) {
    prim_t token = look();

    if (token.t == TAG_RPAREN) {
      get(); // eat the ')'
      return t;
    }

    if (token.t == TAG_END_OF_FILE) {
      return err;
    }

    if (token.t == TAG_DOT) {
      get();        // eat dot
      *p = parse(); // parse the expression after the dot
      if (look().t != TAG_RPAREN) {
        return err;
      }
      get(); // eat the ')'
      return t;
    }

    expr_t res = parse();

    if (!equ(res, nop)) {
      *p = cons(res, nil);
      p = &cell[sp--]; // move to the next cell in the list
    }
  }
}
