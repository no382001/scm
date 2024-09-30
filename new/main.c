#include <stdio.h>

#include "parser.h"

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int paren_count;

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
      prim_t r = {.t = NEWLINE};
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
