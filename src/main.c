#include <stdio.h>

#include "interpreter.h"
#include "parser.h"
#include "print.h"

I ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb, NIL = 0x7ffc,
  MACR = 0x7ffd, NOP = 0x7ffe, VECTOR = 0x7fff;

L cell[N] = {0};
I hp = 0;
I sp = N;
L nil, tru, nop, err, env = 0;

prim_t token_buffer[TOKEN_BUFFER_SIZE] = {0};

ERROR_STATE g_err_state = {NONE, 0, 0};
rcso_struct_t rcso_ctx = {0};
int suppress_jumps, trace_depth, stepping = 0;
int trace = 1;
L T(L x) { return *(unsigned long long *)&x >> 48; }

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int paren_count;

jmp_buf jb;

void init_interpreter() {
  int i;
  nil = box(NIL, 0);
  err = atom("ERR");
  nop = box(NOP, 0);
  tru = atom("#t");
  env = pair(tru, tru, nil);
  for (i = 0; prim[i].s; ++i) {
    env = pair(atom(prim[i].s), box(PRIM, i), env);
  }
}

#ifndef UNITY_TEST
extern int token_idx;
int main(int argc, char **argv) {
  default_ctx.file = stdin;
  if (argc > 2 && strcmp(argv[1], "-e") == 0) {
    switch_ctx_inject_string(argv[2]);
  } else if (argc > 2 && strcmp(argv[1], "-f") == 0) {
    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
      perror("error opening file");
      return 1;
    }
    switch_ctx_to_file(file);
  } else {
    curr_ctx = &default_ctx;
  }

  init_interpreter();

  int i = 0;
  advance();
  while (1) {
    if (looking_at() == EOF) {
      if (curr_ctx->file != stdin) {
        switch_ctx_to_stdin(curr_ctx);
        flush();
      } else {
        break;
      }
    }

    while (looking_at() != '\n' && looking_at() != EOF) {
      token_buffer[i++] = scan();
      print_token(token_buffer[i - 1]);
      fflush(stdout);
    }

    if (looking_at() == '\n') {
      prim_t r = {.t = t_NEWLINE};
      token_buffer[i++] = r;
      print_token(token_buffer[i - 1]);
    }

    int jmpres = setjmp(jb);
    L res = nop;
    if (jmpres == 1) {
      print_and_reset_error();
      gc();
    } else if (jmpres == 2) {
      res = eval(rcso_ctx.x, rcso_ctx.e);
    } else {
      printf("\n%u>", sp - hp / 8);
      res = eval(parse(), env);
    }
    if (!equ(err, res) && !equ(err, nop)) {
      print(res);
      putchar('\n');
    }

    token_idx = 0;
    memset(token_buffer, 0, sizeof(token_buffer));

    flush();
    advance();
  }

  printf("size: %d\n", i);
  return 0;
}

#endif