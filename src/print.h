#pragma once
#include "interpreter.h"
#include "parser.h"

void print_token(prim_t token);
void print(expr_t x, interpreter_t *ctx);
void printlist(expr_t t, interpreter_t *ctx);
void print_stack(int n, interpreter_t *ctx);
void print_heap(interpreter_t *ctx);