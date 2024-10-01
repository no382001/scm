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
  else if (T(x) == VECTOR) {
    I start = ord(x);
    // printf("sp %d\n", start);
    I size = cell[--start];
    // printf("size %d\n", size);
    printf("#(");
    for (I i = 0; i < size; ++i) {
      print(cell[start - 1 - i]);
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

void printlist(L t) {
  for (putchar('(');; putchar(' ')) {
    print(car(t));
    if (_not(t = cdr(t)))
      break;
    if (T(t) != CONS) {
      printf(" . ");
      print(t);
      break;
    }
  }
  putchar(')');
}

void print_stack(int n) {
  printf("Stack contents:\n");
  int c = 0;
  for (I i = sp; i < N; i++) {
    if (c > n) {
      return;
    }
    if (cell[i]) {
      printf("cell[%u] = ", i);
      print(cell[i]);
      printf("\n");
      c++;
    }
  }
}

void print_heap() {
  printf("Heap contents:\n");
  for (I i = 0; i < hp; i++) {
    if (cell[i]) {
      printf("cell[%u] = ", i);
      print(cell[i]);
      printf("\n");
    }
  }
}