#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TAG_ERROR = 0,
  TAG_END_OF_FILE,
  TAG_UNKNOWN,
  TAG_RPAREN,
  TAG_LPAREN,
  TAG_QUOTE,
  TAG_QUASIQUOTE,
  TAG_UNQUOTE,
  TAG_NUMBER,
  TAG_ATOM,
  TAG_COMMENT,
  TAG_NEWLINE,
  TAG_TRUE,
  TAG_FALSE,
  TAG_DOT, // this should work in atom still
  TAG_VECTOR,
  TAG_STRING,
} type_t;

typedef struct {
  type_t t;
  char *str;
  double num;
} prim_t;

void print_token(prim_t token);
char looking_at();
char advance();
void flush();
prim_t scan();

#define PARSE_CTX_BUFF_SIZE 1024

typedef struct {
  FILE *file;
  char buffer[PARSE_CTX_BUFF_SIZE];
  int buf_pos;
  int buf_end;
  char curr;
  bool once; // parse only one line
} parse_ctx;

parse_ctx *deep_copy_parse_ctx(const parse_ctx *src);
void switch_ctx_to_file(FILE *file);
void switch_ctx_to_stdin();
void switch_ctx_inject_string(const char *str);

#define TOKEN_BUFFER_SIZE 1024

typedef struct {
  prim_t buffer[TOKEN_BUFFER_SIZE];
  int idx;
} token_buffer_t;

#include "interpreter.h"

typedef struct read_ctx_t read_ctx_t;
struct read_ctx_t {
  int i;
  interpreter_t *ic;
  token_buffer_t *tb;
  bool (*read)(read_ctx_t *);
  int open_parens;
  int f_load_layer;
};

void reseti(token_buffer_t *tb);
prim_t look(token_buffer_t *tb);
prim_t get(token_buffer_t *tb);
prim_t next(token_buffer_t *tb);
prim_t prev(token_buffer_t *tb);
expr_t parse(interpreter_t *ctx, token_buffer_t *tb);
expr_t list(interpreter_t *ctx, token_buffer_t *tb);

bool read_line(read_ctx_t *rctx);
bool read_multi_line(read_ctx_t *rctx);