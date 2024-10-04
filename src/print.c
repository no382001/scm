#include "print.h"
#include "interpreter.h"
#include "parser.h"
#include <stdio.h>

void print_token(prim_t token) {
  switch (token.t) {
  case t_LPAREN:
    printf("LPAREN ");
    break;
  case t_RPAREN:
    printf("RPAREN ");
    break;
  case t_QUOTE:
    printf("QUOTE ");
    break;
  case t_QUASIQUOTE:
    printf("QUASIQUOTE ");
    break;
  case t_UNQUOTE:
    printf("UNQUOTE ");
    break;
  case t_DOUBLEQUOTE:
    printf("DOUBLEQUOTE ");
    break;
  case t_NUMBER:
    printf("NUMBER:%lf ", token.num);
    break;
  case t_ATOM:
    printf("ATOM:%s ", token.str);
    break;
  case t_UNKNOWN:
    printf("UNKNOWN ");
    break;
  case t_END_OF_FILE:
    printf("EOF ");
    break;
  case t_COMMENT:
    printf("COMMENT ");
    break;
  case t_NEWLINE:
    printf("NEWLINE ");
    break;
  case t_TRUE:
    printf("TRUE ");
    break;
  case t_FALSE:
    printf("FALSE ");
    break;
  case t_DOT:
    printf("DOT ");
    break;
  default:
    break;
  }
}

void print(expr_t x, interpreter_t *ctx) {
  if (ctx->noprint) {
    return;
  }
  if (T(x) == NIL)
    printf("()");
  else if (T(x) == ATOM)
    printf("%s", ic_atomheap + ord(x));
  else if (T(x) == PRIM)
    printf("<%s>", ic_prim[ord(x)].s);
  else if (T(x) == CONS || T(x) == MACR)
    printlist(x, ctx);
  else if (T(x) == CLOS)
    printf("{%u}", ord(x));
  else if (T(x) == VECTOR) {
    tag_t start = ord(x);
    // printf("sp %d\n", start);
    tag_t size = ic_cell[--start];
    // printf("size %d\n", size);
    printf("#(");
    for (tag_t i = 0; i < size; ++i) {
      print(ic_cell[start - 1 - i], ctx);
      if (i < size - 1) {
        printf(" ");
      }
    }
    printf(")");
    // print_stack(10);
  } else if (T(x) == NOP)
    return;
  else
    printf("%.10lg", x);
}

void printlist(expr_t t, interpreter_t *ctx) {
  low_level_ctx_t *llc = &ctx->llc;
  for (putchar('(');; putchar(' ')) {
    print(car(t, llc), ctx);
    if (_not(t = cdr(t, llc)))
      break;
    if (T(t) != CONS) {
      printf(" . ");
      print(t, ctx);
      break;
    }
  }
  putchar(')');
}

void print_stack(int n, interpreter_t *ctx) {
  printf("Stack contents:\n");
  int c = 0;
  for (tag_t i = ic_sp; i < N; i++) {
    if (c > n) {
      return;
    }
    if (ic_cell[i]) {
      printf("cell[%u] = ", i);
      print(ic_cell[i], ctx);
      printf("\n");
      c++;
    }
  }
}

void print_heap(interpreter_t *ctx) {
  printf("Heap contents:\n");
  for (tag_t i = 0; i < ic_hp; i++) {
    if (ic_cell[i]) {
      printf("cell[%u] = ", i);
      print(ic_cell[i], ctx);
      printf("\n");
    }
  }
}