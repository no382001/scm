#include "tinyscheme.h"

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