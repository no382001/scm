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
