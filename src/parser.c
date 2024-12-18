#include "parser.h"

parse_ctx default_ctx = {.file = NULL,
                         .buffer = {0},
                         .buf_pos = 0,
                         .buf_end = 0,
                         .curr = ' ',
                         .injected = false};

parse_ctx *curr_ctx = &default_ctx; // not cool

parse_ctx *deep_copy_parse_ctx(const parse_ctx *src) {
  parse_ctx *new_ctx = malloc(sizeof(parse_ctx));
  if (!new_ctx) {
    return NULL;
  }

  new_ctx->buf_pos = src->buf_pos;
  new_ctx->buf_end = src->buf_end;
  new_ctx->curr = src->curr;

  new_ctx->file = src->file;

  memcpy(new_ctx->buffer, src->buffer, sizeof(src->buffer));

  return new_ctx;
}

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
  curr_ctx->injected = true;
}

char peek() {
  char c = EOF;
  if (curr_ctx->file == stdin) {
    c = fgetc(curr_ctx->file);
    ungetc(c, curr_ctx->file);
  } else if (curr_ctx->buf_pos < curr_ctx->buf_end) {
    c = curr_ctx->buffer[curr_ctx->buf_pos];
  }
  return c;
}

char looking_at() { return curr_ctx->curr; }

char advance() {
  if (curr_ctx->file == stdin) {
    curr_ctx->curr = getchar(); // fuck
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
    res.t = TAG_END_OF_FILE;
    return res;
  }

  switch (looking_at()) {
  case 0:
    res.t = TAG_END_OF_FILE;
    return res;
  case '(':
    advance();
    res.t = TAG_LPAREN;
    paren_count++;
    return res;
  case ')':
    advance();
    paren_count--;
    res.t = TAG_RPAREN;
    return res;
  case '\'':
    advance();
    res.t = TAG_QUOTE;
    return res;
  case '`':
    advance();
    res.t = TAG_QUASIQUOTE;
    return res;
  case ',':
    advance();
    res.t = TAG_UNQUOTE;
    return res;
  case '\n':
    advance();
    res.t = TAG_NEWLINE;
    return res;
  case ';':
    advance();
    while (looking_at() != '\n' && looking_at() != EOF) {
      advance();
    }
    res.t = TAG_COMMENT;
    return res;
  case '"': {
    int i = 0;
    char buff[256] = {0};
    char c = advance();
    do {
      buff[i++] = c;
      c = advance();
    } while (c != '"' && c != EOF);
    buff[i] = '\0';
    advance();

    res.t = TAG_STRING;
    res.str = malloc(strlen(buff) + 1);
    strcpy(res.str, buff);
    return res;
    break;
  }
  case '#':
    advance();
    if (looking_at() == '(') {
      advance();
      paren_count++;
      res.t = TAG_VECTOR;
    } else if (looking_at() == 't') {
      advance();
      res.t = TAG_TRUE;
    } else if (looking_at() == 'f') {
      advance();
      res.t = TAG_FALSE;
    } else {
      res.t = TAG_ERROR;
    }
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

      res.t = TAG_NUMBER;
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

    if (strcmp(buffer, ".") == 0) {
      res.t = TAG_DOT;
    } else {
      res.t = TAG_ATOM;
      res.str = malloc(strlen(buffer) + 1);
      strcpy(res.str, buffer);
    }

    return res; // default is TAG_ERROR
  }
}
