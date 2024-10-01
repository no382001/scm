#pragma once
#include "interpreter.h"
#include "parser.h"

void print_token(prim_t token);
void print(L x);
void printlist(L t);
void print_stack(int n);
void print_heap();