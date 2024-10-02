#pragma once
#include "interpreter.h"
#include "parser.h"

void print_token(prim_t token);
void print(expr_t x);
void printlist(expr_t t);
void print_stack(int n);
void print_heap();