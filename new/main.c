#include <stdio.h>

#include "parser.h"

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;

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
  prim_t tokens[1024] = {};

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
      tokens[i++] = scan();
      print_token(tokens[i-1]);
      fflush(stdout);
    }
    printf("\n");

    flush();
    i = 0;

    advance();
  }

  return 0;
}
