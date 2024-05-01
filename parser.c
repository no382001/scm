#include "tinyscheme.h"

L Read() {
  if (see == EOF){
    return nil;
  }
  return scan(), parse();
}


void skip_multiline_comment() {
  get(); // consume the '|' after '#'
  while (1) {
    if (see == '|') {
      get(); // consume the '|'
      if (see == '#') {
        get(); // consume the '#', end of comment
        return;
      }
    } else {
      get(); // continue reading through the comment
    }
  }
}

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

I seeing(char c) { return c == ' ' ? see > 0 && see <= c : see == c; }

char get() {
  char c = see;
  look();
  return c;
}

char scan() {
  int i = 0;
  while (seeing(' ')){
    look();
  }
  // multiline comments
  if (see == '#') { // uhmm, this might have some consequences no?
    get();
    if (see == '|') {
      skip_multiline_comment();
      return scan();
    }
  // single line comment
  } else if (seeing(';')) {
    while (!seeing('\n') && see != EOF) {
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
