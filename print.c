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
  else if (T(x) == NOP)
    return;
  else
    printf("%.10lg", x);
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