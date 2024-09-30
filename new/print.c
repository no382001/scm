#include "parser.h"
#include <stdio.h>

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
    printf("NUMBER:%lf ", token.num);
    break;
  case ATOM:
    printf("ATOM:%s ", token.str);
    break;
  case UNKNOWN:
    printf("UNKNOWN ");
    break;
  case END_OF_FILE:
    printf("EOF ");
    break;
  case COMMENT:
    printf("COMMENT ");
    break;
  case NEWLINE:
    printf("NEWLINE ");
    break;
  default:
    break;
  }
}