#include "tinyscheme.h"

void switch_ctx_to_file(FILE * file) {
    curr_ctx = (parsing_ctx*)malloc(sizeof(parsing_ctx));
    curr_ctx->file = file;
    curr_ctx->buf_pos = 0;
    curr_ctx->buf_end = 0;
    curr_ctx->see = ' ';
    look();  // init first char
}

void switch_ctx_to_stdin() {
    if (curr_ctx != &default_ctx) {
        if (curr_ctx->file) {
            fclose(curr_ctx->file);
        }
        free(curr_ctx);
    }

    curr_ctx = &default_ctx;
}

L Read() {
  if (curr_ctx->see == EOF){
    return nil;
  }
  char c = scan();
  if (c == EOF){
    return err; // i cant think of nothing better rn
  }
  return parse();
}

void skip_multiline_comment() {
  get(); // consume the '|' after '#'
  while (1) {
    if (curr_ctx->see == '|') {
      get(); // consume the '|'
      if (curr_ctx->see == '#') {
        get(); // consume the '#', end of comment
        return;
      }
    } else {
      get(); // continue reading through the comment
    }
  }
}

/** /
void look() {
  int c = getchar();
  if (c == EOF) {
    // check if EOF is due to an actual end of file condition
    if (feof(stdin)) {
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);
        clearerr(stdin);  // clear EOF condition on stdin
        see = ' ';
        return;     // return to avoid setting see to EOF
      } else {
        exit(1);  // exit if EOF on the original stdin and not processing a file
    }
  }
  see = c;
}
/ **/

void look() {
    if (curr_ctx->file == stdin) {
        // default, read from stdin
        int c = getchar();
        curr_ctx->see = c == EOF ? EOF : (char)c;
    } else if (curr_ctx->buf_pos < curr_ctx->buf_end) {
        // advance the buffer
        curr_ctx->see = curr_ctx->buffer[curr_ctx->buf_pos++];
    } else if (curr_ctx->file != NULL) {
        // attempt read
        curr_ctx->buf_end = fread(curr_ctx->buffer, 1, sizeof(curr_ctx->buffer), curr_ctx->file);
        curr_ctx->buf_pos = 0;

        if (curr_ctx->buf_end > 0) {
            // continue with new data
            curr_ctx->see = curr_ctx->buffer[curr_ctx->buf_pos++];
        } else {
            if (feof(curr_ctx->file)){
                curr_ctx->see = EOF;
            }
        }
    } else {
        curr_ctx->see = EOF; // no file?
    }
}


I seeing(char c) { return c == ' ' ? curr_ctx->see > 0 && curr_ctx->see <= c : curr_ctx->see == c; }

char get() {
  char c = curr_ctx->see;
  look();
  return c;
}

char scan() {
  int i = 0;
  while (seeing(' ')){
    look();
  }
  if (curr_ctx->see == EOF){
    return EOF;
  }
  // multiline comments
  if (curr_ctx->see == '#') {
    get();
    if (curr_ctx->see == '|') {
      skip_multiline_comment();
      return scan();
    }
  // single line comment
  } else if (seeing(';')) {
    while (!seeing('\n') && curr_ctx->see != EOF) {
      look();
    }
    return scan();
  } else if (seeing('(') || seeing(')') || seeing('\'')){
    char c = get();
    buf[i++] = c;
  } else {
    do {
      char c = get();
      buf[i++] = c;
    } while (i < PARSE_BUFFER-1 && !seeing('(') && !seeing(')') && !seeing(' '));
  }
  return buf[i] = 0, *buf;
}

L list() {
  L t, *p;
  for (t = nil, p = &t;; *p = cons(parse(), nil), p = cell + sp)
  {
    if (scan() == ')')
      return t;
    if (*buf == '.' && !buf[1])
      return *p = Read(), scan(), t;
  }
}

L parse() {
  L n;
  int i;
  if (*buf == '(')
    return list();
  if (*buf == '\'')
    return cons(atom("quote"), cons(Read(), nil));
  return sscanf(buf, "%lg%n", &n, &i) > 0 && !buf[i] ? n : atom(buf);
}

