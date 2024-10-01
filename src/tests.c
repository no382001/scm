#include "../unity/src/unity.h"
#include "interpreter.h"
#include "parser.h"
#include "print.h"

#include <stdbool.h>

void init_interpreter();
extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int token_idx;

void setUp(void) {
  trace = 0;

  g_err_state.type = NONE;
  g_err_state.proc = 0;
  g_err_state.box = 0;

  curr_ctx->file = NULL;
  memset(curr_ctx->buffer, 0, sizeof(curr_ctx->buffer));
  curr_ctx->buf_pos = 0;
  curr_ctx->buf_end = 0;
  curr_ctx->curr = ' ';

  init_interpreter();
}
int doprint = 0;
void tearDown(void) {
  doprint = 0;
  hp = 0;
  sp = N;
  memset(token_buffer, 0, sizeof(token_buffer));
  token_idx = 0;
}

void printlist_to_buffer(L t, char *buffer, size_t buffer_size);
void lisp_expression_to_string(L x, char *buffer, size_t buffer_size);

// look aat this fucking mess up here
void print_to_buffer(L x, char *buffer, size_t buffer_size) {
  if (T(x) == NIL) {
    snprintf(buffer, buffer_size, "()");
  } else if (T(x) == ATOM) {
    snprintf(buffer, buffer_size, "%s", A + ord(x));
  } else if (T(x) == PRIM) {
    snprintf(buffer, buffer_size, "<%s>", prim[ord(x)].s);
  } else if (T(x) == CONS || T(x) == MACR) {
    printlist_to_buffer(x, buffer, buffer_size);
  } else if (T(x) == CLOS) {
    snprintf(buffer, buffer_size, "{%u}", ord(x));
  } else if (T(x) == VECTOR) {
    I start = ord(x);
    I size = cell[--start];
    size_t offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "#(");
    for (I i = 0; i < size; ++i) {
      char temp[128];
      print_to_buffer(cell[start - 1 - i], temp, sizeof(temp));
      offset += snprintf(buffer + offset, buffer_size - offset, "%s", temp);
      if (i < size - 1) {
        offset += snprintf(buffer + offset, buffer_size - offset, " ");
      }
    }
    snprintf(buffer + offset, buffer_size - offset, ")");
  } else if (T(x) == NOP) {
    return;
  } else {
    snprintf(buffer, buffer_size, "%.10lg", x);
  }
}

void printlist_to_buffer(L t, char *buffer, size_t buffer_size) {
  size_t offset = 0;
  offset += snprintf(buffer + offset, buffer_size - offset, "(");

  while (1) {
    char temp[128];
    print_to_buffer(car(t), temp, sizeof(temp));
    offset += snprintf(buffer + offset, buffer_size - offset, "%s", temp);

    t = cdr(t);

    if (_not(t)) {
      break;
    }
    if (T(t) != CONS) {
      offset += snprintf(buffer + offset, buffer_size - offset, " . ");
      print_to_buffer(t, temp, sizeof(temp));
      offset += snprintf(buffer + offset, buffer_size - offset, "%s", temp);
      break;
    }
    offset += snprintf(buffer + offset, buffer_size - offset, " ");
  }

  snprintf(buffer + offset, buffer_size - offset, ")");
}

bool match_variable(L x, const char *str) {
  char debug_output[256];

  if (T(x) == ATOM) {
    const char *var = A + ord(x);
    sprintf(debug_output, "comparing Atom: %s with %s", var, str);
    printf("%s\n", debug_output);
    return strcmp(var, str) == 0;

  } else if (T(x) == NIL) {
    sprintf(debug_output, "comparing NIL with %s", str);
    printf("%s\n", debug_output);
    return strcmp("()", str) == 0;

  } else if (T(x) == CONS) {
    char buffer[128];
    printlist_to_buffer(x, buffer, sizeof(buffer));
    sprintf(debug_output, "comparing CONS: %s with %s", buffer, str);
    printf("%s\n", debug_output);
    return strcmp(buffer, str) == 0;
  } else if (T(x) == VECTOR) {
    I start = ord(x);
    I size = cell[--start];

    char vec_str[1024] = "#(";

    for (I i = 0; i < size; ++i) {
      char elem_str[128];
      lisp_expression_to_string(cell[start - 1 - i], elem_str,
                                sizeof(elem_str));

      strcat(vec_str, elem_str);
      if (i < size - 1) {
        strcat(vec_str, " ");
      }
    }
    strcat(vec_str, ")");

    sprintf(debug_output, "comparing VECTOR: %s with %s", vec_str, str);
    printf("%s\n", debug_output);
    return strcmp(vec_str, str) == 0;
  }

  return false;
}

void lisp_expression_to_string(L x, char *buffer, size_t buffer_size) {
  if (T(x) == ATOM) {
    const char *var = A + ord(x);
    snprintf(buffer, buffer_size, "%s", var);
  } else if (T(x) == NIL) {
    snprintf(buffer, buffer_size, "()");
  } else if (T(x) == CONS) {
    char left[128], right[128];
    lisp_expression_to_string(car(x), left, sizeof(left));
    lisp_expression_to_string(cdr(x), right, sizeof(right));
    snprintf(buffer, buffer_size, "(%s . %s)", left, right);
  } else {
    // for numbers or other types
    snprintf(buffer, buffer_size, "%.10g", x);
  }
}

L parse_this(const char *str) {
  memset(token_buffer, 0, sizeof(token_buffer));
  token_idx = 0;
  switch_ctx_inject_string(str);

  int i = 0;
  advance();
  while (1) {
    if (looking_at() == EOF) {
      break;
    }

    while (looking_at() != '\n' && looking_at() != EOF) {
      token_buffer[i++] = scan();
      if (doprint) {
        print_token(token_buffer[i - 1]);
        fflush(stdout);
      }
    }

    if (looking_at() == '\n') {
      prim_t r = {.t = t_NEWLINE};
      token_buffer[i++] = r;
      if (doprint) {
        print_token(token_buffer[i - 1]);
      }
    }
    return parse();
  }

  return nop;
}

/* --- parsing --- */

void test_simple1(void) {
  L result = parse_this("(+ 1 1)\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 1)"), true);
}

void test_simple2(void) {
  L result = parse_this("(+ 1 2 3)\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 2 3)"), true);
}

void test_nested1(void) {
  L result = parse_this("(+ (* 2 3) (- 5 2))\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ (* 2 3) (- 5 2))"), true);
}

void test_nested2(void) {
  L result = parse_this("(+ (* 2 (+ 3 4)) (- 10 (+ 2 (* 1 3))))");
  TEST_ASSERT_EQUAL(
      match_variable(result, "(+ (* 2 (+ 3 4)) (- 10 (+ 2 (* 1 3))))"), true);
}

void test_nested3(void) {
  L result = parse_this("(+ (list 1 (list 2 3)) (list 4 5))");
  TEST_ASSERT_EQUAL(
      match_variable(result, "(+ (list 1 (list 2 3)) (list 4 5))"), true);
}

void test_empty(void) {
  L result = parse_this("(+ 1 ())");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 ())"), true);
}

/* --- eval --- */
L eval_this(const char *str) { return eval(parse_this(str), env); }

void test_define(void) {
  L result = eval_this("(define n 1)");
  TEST_ASSERT_EQUAL(match_variable(result, "n"), true);
  result = eval_this("(+ n 1)");
  TEST_ASSERT_EQUAL(result, 2);
  print(result);
  putchar('\n');
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_simple1);
  RUN_TEST(test_simple2);
  RUN_TEST(test_nested1);
  RUN_TEST(test_nested2);
  RUN_TEST(test_nested3);
  RUN_TEST(test_empty);

  RUN_TEST(test_define);
  return UNITY_END();
}
