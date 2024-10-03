#include <stdio.h>

#include "interpreter.h"
#include "parser.h"
#include "print.h"

const tag_t ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb,
            NIL = 0x7ffc, MACR = 0x7ffd, NOP = 0x7ffe, VECTOR = 0x7fff;

// expr_t cell[N] = {0};
// tag_t hp = 0;
// tag_t sp = N;
// expr_t nil, tru, nop, err, env = 0;

prim_t token_buffer[TOKEN_BUFFER_SIZE] = {0};

expr_t T(expr_t x) { return *(unsigned long long *)&x >> 48; }

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int paren_count;

extern prim_procs_t prim[];

void init_interpreter(interpreter_t *ctx) {

  ic_sp = N;
  ic_hp = 0;

  ic_c_nil = box(NIL, 0);
  ic_c_err = atom("ERR", ic2llcref);
  ic_c_nop = box(NOP, 0);
  ic_c_tru = atom("#t", ic2llcref);

  ic_env = pair(ic_c_tru, ic_c_tru, ic_c_nil, ic2llcref);

  ic_prim = prim;

  for (int i = 0; ic_prim[i].s; ++i) {
    ic_env =
        pair(atom(ic_prim[i].s, ic2llcref), box(PRIM, i), ic_env, ic2llcref);
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
    curr_ctx =
        &default_ctx; // this should just be a config you can never change
  }

  interpreter_t intrp = {0};
  init_interpreter(&intrp);
  interpreter_t *ctx = &intrp;

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

    int jmpres = setjmp(ctx->llc.jumps.jb);
    expr_t res = ic_c_nop;
    if (jmpres == 1) {
      print_and_reset_error(ctx);
      gc(ctx);
    } else if (jmpres == 2) {
      res = eval(ic_rcso.x, ic_rcso.e, ctx);
    } else {
      printf("\n%u>", ic_sp - ic_hp / 8);
      res = eval(parse(ctx), ic_env, ctx);
    }
    if (!equ(ic_c_err, res) && !equ(ic_c_err, ic_c_nop)) {
      print(res, ctx);
      putchar('\n');
    }

    i = 0;
    token_idx = 0;
    memset(token_buffer, 0, sizeof(token_buffer));

    flush();
    advance();
  }

  printf("size: %d\n", i);
  return 0;
}

#endif