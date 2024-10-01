#include <stdio.h>

#include "interpreter.h"
#include "parser.h"

I ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa, CLOS = 0x7ffb, NIL = 0x7ffc,
  MACR = 0x7ffd, NOP = 0x7ffe, VECTOR = 0x7fff;

L cell[N] = {0};
I hp, sp = 0;
L nil, tru, nop, err, env = 0;

ERROR_STATE g_err_state = {NONE, 0, 0};
rcso_struct_t rcso_ctx = {0};
int suppress_jumps, trace_depth, trace, stepping = 0;

L T(L x) { return *(unsigned long long *)&x >> 48; }

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int paren_count;

jmp_buf jb;

int main(int argc, char **argv) {
  default_ctx.file = stdin;
  if (argc > 1) {
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      perror("error opening file");
      return 1;
    }
    switch_ctx_to_file(file);
  } else {
    curr_ctx = &default_ctx;
  }

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
    static prim_t tokens[1024] = {0};

    while (looking_at() != '\n' && looking_at() != EOF) {
      tokens[i++] = scan();
      print_token(tokens[i - 1]);
      fflush(stdout);
    }

    if (looking_at() == '\n') {
      prim_t r = {.t = t_NEWLINE};
      tokens[i++] = r;
      print_token(tokens[i - 1]);
    }

    printf("\n");

    flush();
    advance();
  }

  printf("size: %d\n", i);
  return 0;
}
