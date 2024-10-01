#include "parser.h"

parse_ctx default_ctx = {
    .file = NULL, .buffer = {0}, .buf_pos = 0, .buf_end = 0, .curr = ' '};

parse_ctx *curr_ctx = &default_ctx;

void switch_ctx_to_file(FILE *file) {
  curr_ctx->file = file;
  curr_ctx->buf_pos = 0;
  curr_ctx->buf_end = 0;
  curr_ctx->curr = ' ';
}

void switch_ctx_to_stdin() {
  if (curr_ctx->file != stdin) {
    if (curr_ctx->file) {
      fclose(curr_ctx->file);
    }
    curr_ctx->file = stdin;
  }
}

void switch_ctx_inject_string(const char *input_str) {
  strncpy(curr_ctx->buffer, input_str, sizeof(curr_ctx->buffer) - 1);
  curr_ctx->buffer[sizeof(curr_ctx->buffer) - 1] = '\0';
  curr_ctx->buf_pos = 0;
  curr_ctx->buf_end = strlen(curr_ctx->buffer);
  curr_ctx->curr = curr_ctx->buffer[0];
  curr_ctx->file = NULL;
}

char peek() {
  char c = EOF;
  if (curr_ctx->file == stdin) {
    c = fgetc(curr_ctx->file);
    ungetc(c, curr_ctx->file);
  } else if (curr_ctx->buf_pos + 1 < curr_ctx->buf_end) {
    c = curr_ctx->buffer[curr_ctx->buf_pos + 1];
  }
  return c;
}

char looking_at() { return curr_ctx->curr; }

char advance() {
  if (curr_ctx->file == stdin) {
    curr_ctx->curr = getchar();
    return curr_ctx->curr;
  } else if (curr_ctx->buf_pos < curr_ctx->buf_end) {
    // advance the buffer
    curr_ctx->curr = curr_ctx->buffer[curr_ctx->buf_pos++];
  } else if (curr_ctx->file != NULL) {
    // attempt read
    curr_ctx->buf_end =
        fread(curr_ctx->buffer, 1, sizeof(curr_ctx->buffer), curr_ctx->file);
    curr_ctx->buf_pos = 0;

    if (curr_ctx->buf_end > 0) {
      // continue with new data
      curr_ctx->curr = curr_ctx->buffer[curr_ctx->buf_pos++];
    } else {
      if (feof(curr_ctx->file)) {
        curr_ctx->curr = EOF;
      }
    }
  } else {
    curr_ctx->curr = EOF; // no file?
  }

  return curr_ctx->curr;
}

void flush() { curr_ctx->curr = ' '; }

int paren_count = 0;

prim_t scan() {
  prim_t res = {0};

  while (isspace(looking_at())) {
    advance();
  }

  if (looking_at() == EOF) {
    res.t = t_END_OF_FILE;
    return res;
  }

  switch (looking_at()) {
  case '(':
    advance();
    res.t = t_LPAREN;
    paren_count++;
    return res;
  case ')':
    advance();
    paren_count--;
    res.t = t_RPAREN;
    return res;
  case '\'':
    advance();
    res.t = t_QUOTE;
    return res;
  case '`':
    advance();
    res.t = t_QUASIQUOTE;
    return res;
  case ',':
    advance();
    res.t = t_UNQUOTE;
    return res;
  case '"':
    advance();
    res.t = t_DOUBLEQUOTE;
    return res;
  case '\n': // never hit, probably
    advance();
    res.t = t_NEWLINE;
    return res;
  case ';':
    advance();
    while (looking_at() != '\n' && looking_at() != EOF) {
      advance();
    }
    res.t = t_COMMENT;
    return res;
  default:
    // number
    if (isdigit(looking_at()) || (looking_at() == '-' && isdigit(peek()))) {

      char buffer[64];
      int idx = 0;
      if (looking_at() == '-') {
        buffer[idx++] = looking_at();
        advance();
      }

      while ((isdigit(looking_at()) || looking_at() == '.')) {
        buffer[idx++] = looking_at();
        advance();
      }
      buffer[idx] = '\0';

      res.t = t_NUMBER;
      res.num = strtod(buffer, NULL);
      return res;
    }

    char buffer[128];
    int idx = 0;
    while (looking_at() != ' ' && looking_at() != EOF && looking_at() != ')' &&
           looking_at() != '\n') {
      buffer[idx++] = looking_at();
      advance();
    }
    buffer[idx] = '\0';

    if (strcmp(buffer, "#t") == 0) {
      res.t = t_TRUE;
    } else if (strcmp(buffer, "#f") == 0) {
      res.t = t_FALSE;
    } else if (strcmp(buffer, ".") == 0) {
      res.t = t_DOT;
    } else {
      res.t = t_ATOM;
      res.str = malloc(strlen(buffer) + 1);
      strcpy(res.str, buffer);
    }

    return res;
  }
}
