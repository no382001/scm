#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  char curr;
  char *history;
  int pos;
  int size;
} p_ctx;

static p_ctx *c = NULL;

enum type_t {
  UNKNOWN = 0,
  DOUBLEQUOTE,
  RPAREN,
  LPAREN,
  QUOTE,
  QUASIQUOTE,
  UNQUOTE,
  NUMBER,
  ATOM,
  ERROR
};

typedef struct {
  enum type_t t;
  char *str;
  double num;
} prim_t;

void print_token(prim_t token);

char looking_at() { return c->curr; }

char advance() {
  c->curr = getchar();
  return c->curr;
}

void flush() {
  memset(c->history, 0, c->size);
  c->pos = 0;
}

static int paren_count = 0;

prim_t scan() {
  prim_t res = {0};
  while (isspace(looking_at())) {
    advance();
  }

  if (looking_at() == EOF) {
    res.t = EOF;
    return res;
  }

  switch (looking_at()) {
  case '(':
    advance();
    res.t = LPAREN;
    paren_count++;
    return res;
  case ')':
    advance();
    paren_count--;
    res.t = RPAREN;
    return res;
  case '\'':
    advance();
    res.t = QUOTE;
    return res;
  case '`':
    advance();
    res.t = QUASIQUOTE;
    return res;
  case ',':
    advance();
    res.t = UNQUOTE;
    return res;
  case '"':
    advance();
    res.t = DOUBLEQUOTE;
    return res;
  default:
    // number
    if (isdigit(looking_at()) || looking_at() == '-') {
      char buffer[64];
      int idx = 0;

      while ((isdigit(looking_at()) || looking_at() == '.') &&
             idx < sizeof(buffer) - 1) {
        buffer[idx++] = looking_at();
        advance();
      }
      buffer[idx] = '\0';

      res.t = NUMBER;
      res.num = strtod(buffer, NULL);
      return res;
    }
    // atom
    char buffer[128];
    int idx = 0;
    bool b = false;
    while (looking_at() != ' ' && looking_at() != EOF && looking_at() != ')') {
      buffer[idx++] = looking_at();
      advance();
    }
    buffer[idx] = '\0';

    res.t = ATOM;
    res.str = malloc(strlen(buffer) + 1);
    strcpy(res.str, buffer);
    return res;
  }
}

void print_token(prim_t token) {
  switch (token.t) {
  case LPAREN:
    printf("LPAREN ");
    break;
  case RPAREN:
    printf("RPAREN ");
    break;
  case QUOTE:
    printf("QUOTE ");
    break;
  case QUASIQUOTE:
    printf("QUASIQUOTE ");
    break;
  case UNQUOTE:
    printf("UNQUOTE ");
    break;
  case DOUBLEQUOTE:
    printf("DOUBLEQUOTE ");
    break;
  case NUMBER:
    printf("<NUMBER:%lf> ", token.num);
    break;
  case ATOM:
    printf("<ATOM:%s> ", token.str);
    break;
  case UNKNOWN:
    printf("UNKNOWN ");
    break;
  case EOF:
    printf("EOF ");
    break;
  default:
    break;
  }
}

int main() {
  p_ctx x = {.curr = 0, .size = 128, .history = malloc(128), .pos = 0};
  c = &x;

  int i = 0;
  prim_t tokens[128] = {};

  advance();
  while (1) {
    while (looking_at() != EOF) {
      tokens[i++] = scan();
      if (paren_count == 0) {
        break;
      }
    }

    for (int k = 0; k < i; k++) {
      print_token(tokens[k]);
    }
    flush();
    i = 0;
  }

  return 0;
}

// do something with EOF and EOL, so it can parse more easy
// parens checking, thats the only thing that can fuck things up, the rest is
// jit
