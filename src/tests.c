#include "../unity/src/unity.h"
#include "interpreter.h"
#include "parser.h"
#include "print.h"

#include <stdbool.h>

extern parse_ctx *curr_ctx;
extern parse_ctx default_ctx;
extern int token_idx;
extern prim_t token_buffer[TOKEN_BUFFER_SIZE];
extern jmp_buf jb;

int doprint = 0;
interpreter_t *ctx;

void init_interpreter(interpreter_t *c);

#define g_err_state ic_errstate
#define err ic_c_err
#define nop ic_c_nop
#define nil ic_c_nil
#define cdr(expr) cdr(expr, ic2llcref)
#define car(expr) car(expr, ic2llcref)
#define cell ic_atomheap

void setUp(void) {
  ctx = (interpreter_t *)calloc(1, sizeof(interpreter_t));
  init_interpreter(ctx);

  token_idx = 0;
  memset(token_buffer, 0, sizeof(token_buffer));
}

void tearDown(void) {
  doprint = 0;
  ic_sp = N;
  ic_hp = 0;
  memset(token_buffer, 0, sizeof(token_buffer));
  token_idx = 0;
  free(ctx);
  ctx = NULL;
}
void print_to_buffer(expr_t x, char *buffer, size_t buffer_size);
void printlist_to_buffer(expr_t t, char *buffer, size_t buffer_size) {
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

void print_to_buffer(expr_t x, char *buffer, size_t buffer_size) {
  if (T(x) == NIL) {
    snprintf(buffer, buffer_size, "()");
  } else if (T(x) == ATOM) {
    snprintf(buffer, buffer_size, "%s", ic_atomheap + ord(x));
  } else if (T(x) == PRIM) {
    snprintf(buffer, buffer_size, "<%s>", ic_prim[ord(x)].s);
  } else if (T(x) == CONS || T(x) == MACR) {
    printlist_to_buffer(x, buffer, buffer_size);
  } else if (T(x) == CLOS) {
    snprintf(buffer, buffer_size, "{%u}", ord(x));
  } else if (T(x) == VECTOR) {
    tag_t start = ord(x);
    tag_t size = ic_cell[--start];
    size_t offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "#(");
    for (tag_t i = 0; i < size; ++i) {
      char temp[128];
      print_to_buffer(ic_cell[start - 1 - i], temp, sizeof(temp));
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
expr_t parse_this(const char *str);

expr_t eval_this(const char *str) {
  expr_t ret = nil;
  int i = setjmp(ic_jb);
  if (i == 1) {
    return err;
  } else {
    ret = eval(parse_this(str), ic_env, ctx);
  }
  return ret;
}
void lisp_expression_to_string(expr_t x, char *buffer, size_t buffer_size);
bool match_variable(expr_t x, const char *str) {
  char debug_output[256];

  if (T(x) == ATOM) {
    const char *var = ic_atomheap + ord(x);
    sprintf(debug_output, "comparing Atom: %s with %s", var, str);
    printf("%s\n", debug_output);
    return strcmp(var, str) == 0;

  } else if (T(x) == NIL) {
    sprintf(debug_output, "comparing NIexpr_t with %s", str);
    printf("%s\n", debug_output);
    return strcmp("()", str) == 0;

  } else if (T(x) == CONS) {
    char buffer[128];
    printlist_to_buffer(x, buffer, sizeof(buffer));
    sprintf(debug_output, "comparing CONS: %s with %s", buffer, str);
    printf("%s\n", debug_output);
    return strcmp(buffer, str) == 0;
  } else if (T(x) == VECTOR) {
    tag_t start = ord(x);
    tag_t size = ic_cell[--start];

    char vec_str[1024] = "#(";

    for (tag_t i = 0; i < size; ++i) {
      char elem_str[128];
      lisp_expression_to_string(ic_cell[start - 1 - i], elem_str,
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
  } else {
    char buffer[50];
    if (x == (int)x) {
      snprintf(buffer, sizeof(buffer), "%d", (int)x);
    } else {
      snprintf(buffer, sizeof(buffer), "%f", x);
    }
    sprintf(debug_output, "comparing Atom: %s with %s", buffer, str);
    printf("%s\n", debug_output);
    return strcmp(buffer, str) == 0;
  }

  return false;
}

void lisp_expression_to_string(expr_t x, char *buffer, size_t buffer_size) {
  if (T(x) == ATOM) {
    const char *var = ic_atomheap + ord(x);
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

expr_t parse_this(const char *str) {
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
    return parse(ctx);
  }

  return nop;
}

/* --- parsing --- */

void test_simple1(void) {
  expr_t result = parse_this("(+ 1 1)\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 1)"), true);
}

void test_simple2(void) {
  expr_t result = parse_this("(+ 1 2 3)\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 2 3)"), true);
}

void test_nested1(void) {
  expr_t result = parse_this("(+ (* 2 3) (- 5 2))\n");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ (* 2 3) (- 5 2))"), true);
}

void test_nested2(void) {
  expr_t result = parse_this("(+ (* 2 (+ 3 4)) (- 10 (+ 2 (* 1 3))))");
  TEST_ASSERT_EQUAL(
      match_variable(result, "(+ (* 2 (+ 3 4)) (- 10 (+ 2 (* 1 3))))"), true);
}

void test_nested3(void) {
  expr_t result = parse_this("(+ (list 1 (list 2 3)) (list 4 5))");
  TEST_ASSERT_EQUAL(
      match_variable(result, "(+ (list 1 (list 2 3)) (list 4 5))"), true);
}

void test_empty(void) {
  expr_t result = parse_this("(+ 1 ())");
  TEST_ASSERT_EQUAL(match_variable(result, "(+ 1 ())"), true);
}

#define ASSERT_VAR(actual, expected)                                           \
  TEST_ASSERT_EQUAL(match_variable(actual, expected), true);

/* --- primitives --- */

void test_define(void) {
  expr_t result = eval_this("(define n 1)");
  ASSERT_VAR(result, "n");
  result = eval_this("(+ n 1)");
  ASSERT_VAR(result, "2");
}

void test_LogicalTrue(void) {
  expr_t result = eval_this("(eq? 1 1)\n");
  ASSERT_VAR(result, "#t");
}

void test_LogicalFalse(void) {
  expr_t result = eval_this("(eq? 1 2)\n");
  ASSERT_VAR(result, "()");
}

void test_ConstructList(void) {
  expr_t result = eval_this("(cons 1 2)\n");
  ASSERT_VAR(result, "(1 . 2)");
}

void test_CarOperation(void) {
  expr_t result = eval_this("(car (cons 1 2))\n");
  TEST_ASSERT_EQUAL(1, result);
}

void test_CdrOperation(void) {
  expr_t result = eval_this("(cdr (cons 1 2))\n");
  TEST_ASSERT_EQUAL(2, result);
}

void test_IfTrue(void) {
  expr_t result = eval_this("(if '(1) 1 2)\n");
  TEST_ASSERT_EQUAL(1, result);
}

void test_IfFalse(void) {
  expr_t result = eval_this("(if () 1 2)\n");
  TEST_ASSERT_EQUAL(2, result);
}

void test_DefineAndUseFunction(void) {
  eval_this("(define square (lambda (x) (* x x)))\n");
  expr_t result = eval_this("(square 3)\n");
  TEST_ASSERT_EQUAL(9, result);
}

void test_LetBinding(void) {
  expr_t result = eval_this("(let* (x 5) (y 3) (+ x y))\n");
  TEST_ASSERT_EQUAL(8, result);
}

void test_UndefinedVariable(void) {
  expr_t result = eval_this("(+ a 1)\n");
  TEST_ASSERT_EQUAL(err, result);
  TEST_ASSERT_EQUAL(ASSOC_VALUE_N_FOUND, g_err_state.type);
}

void test_TypeMismatch(void) {
  expr_t result = eval_this("(+ 'a 1)\n");
  TEST_ASSERT_EQUAL(err, result);

  result = eval_this("(+ (quote a) 1)\n");
  TEST_ASSERT_EQUAL(err, result);
}

void test_Quote(void) {
  expr_t result = eval_this("(quote a)\n");
  ASSERT_VAR(result, "a");

  result = eval_this("'b\n");
  ASSERT_VAR(result, "b");
}

void test_ComplexExpression(void) {
  expr_t result = eval_this("(* (+ 2 3) (- 5 2))\n");
  TEST_ASSERT_EQUAL(15, result);
}

void test_RecursiveFunction(void) {
  eval_this(
      "(define fact (lambda (n) (if (eq? n 1) 1 (* n (fact (- n 1))))))\n");
  expr_t result = eval_this("(fact 5)\n");
  TEST_ASSERT_EQUAL(120, result);
}

void test_HigherOrderFunction(void) {
  eval_this("(define apply-func (lambda (f x) (f x)))\n");
  eval_this("(define inc (lambda (x) (+ x 1)))\n");
  expr_t result = eval_this("(apply-func inc 5)\n");
  TEST_ASSERT_EQUAL(6, result);
}

void test_Addition(void) {
  expr_t result = eval_this("(+ 1 2 3 4)\n");
  TEST_ASSERT_EQUAL(10, result);
}

void test_Subtraction(void) {
  expr_t result = eval_this("(- 10 2 3)\n");
  TEST_ASSERT_EQUAL(5, result);
}

void test_Multiplication(void) {
  expr_t result = eval_this("(* 2 3 4)\n");
  TEST_ASSERT_EQUAL(24, result);
}

void test_Division(void) {
  expr_t result = eval_this("(/ 24 3 2)\n");
  TEST_ASSERT_EQUAL(4, result);
}

void test_Integer(void) {
  expr_t result = eval_this("(int 3.75)\n");
  TEST_ASSERT_EQUAL(3, result);
}

void test_LessThan(void) {
  expr_t result = eval_this("(< 3 4)\n");
  ASSERT_VAR(result, "#t");
}

void test_Or(void) {
  expr_t result = eval_this("(or () #t)\n");
  ASSERT_VAR(result, "#t");

  result = eval_this("(or () ())\n");
  ASSERT_VAR(result, "()");
}

void test_And(void) {
  expr_t result = eval_this("(and () ())\n");
  ASSERT_VAR(result, "()");

  result = eval_this("(and 'a 'a)\n");
  ASSERT_VAR(result, "a");
}

void test_Not(void) {
  expr_t result = eval_this("(not ())\n");
  ASSERT_VAR(result, "#t");
}

void test_Cond(void) {
  expr_t result = eval_this("(cond ((eq? 1 2) 3) ((eq? 2 2) 4))\n");
  TEST_ASSERT_EQUAL(4, result);
}

void test_CreateVector(void) {
  expr_t result = eval_this("(vector 1 2 3 4)\n");
  ASSERT_VAR(result, "#(1 2 3 4)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_VectorRef(void) {
  eval_this("(define v (vector 1 2 3 4))\n");
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 0)\n"), 1);
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 1)\n"), 2);
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 2)\n"), 3);
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 3)\n"), 4);
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_VectorSet(void) {
  eval_this("(define v (vector 1 2 3 4))\n");
  eval_this("(vector-set! v 1 42)\n");
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 1)\n"), 42);

  eval_this("(vector-set! v 0 100)\n");
  TEST_ASSERT_EQUAL(eval_this("(vector-ref v 0)\n"), 100);
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_VectorLength(void) {
  eval_this("(define v (vector 1 2 3 4))\n");
  TEST_ASSERT_EQUAL(eval_this("(vector-length v)\n"), 4);
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_QuasiquoteSimple(void) {
  expr_t result = eval_this("`(1 2 3)\n");
  ASSERT_VAR(result, "(1 2 3)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_QuasiquoteWithUnquote(void) {
  expr_t result = eval_this("`(1 2 ,(+ 3 4) 5)\n");
  ASSERT_VAR(result, "(1 2 7 5)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_NestedQuasiquoteWithUnquote(void) {
  expr_t result = eval_this("`(1 ` , (+ 2 3) ,(+ 4 5))\n");
  ASSERT_VAR(result, "(1 (quasiquote (unquote (+ 2 3))) 9)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);

  result = eval_this("`(1 ` ,(+ 2 3) ,(+ 4 5))\n");
  ASSERT_VAR(result, "(1 (quasiquote (unquote (+ 2 3))) 9)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);

  result = eval_this("`(1 `,(+ 2 3) ,(+ 4 5))\n");
  ASSERT_VAR(result, "(1 (quasiquote (unquote (+ 2 3))) 9)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);

  result = eval_this("(define n 1)\n");
  result = eval_this("`(+ 1 ,n)\n");
  ASSERT_VAR(result, "(+ 1 1)");
  TEST_ASSERT_EQUAL(g_err_state.type, NONE);
}

void test_UnquoteWithoutQuasiquote(void) {
  expr_t result = eval_this(",(+ 2 3)\n");
  TEST_ASSERT_EQUAL(g_err_state.type, UNQUOTE_OUTSIDE_QUASIQUOTE);
}

int main(void) {
  UNITY_BEGIN();

  // simple arithmetic tests
  RUN_TEST(test_Addition);
  RUN_TEST(test_Subtraction);
  RUN_TEST(test_Multiplication);
  RUN_TEST(test_Division);
  RUN_TEST(test_Integer);

  // logical operations
  RUN_TEST(test_LogicalTrue);
  RUN_TEST(test_LogicalFalse);
  RUN_TEST(test_LessThan);
  RUN_TEST(test_Or);
  RUN_TEST(test_And);
  RUN_TEST(test_Not);

  // control flow and conditions
  RUN_TEST(test_IfTrue);
  RUN_TEST(test_IfFalse);
  RUN_TEST(test_Cond);

  // functions and higher-order functions
  RUN_TEST(test_DefineAndUseFunction);
  RUN_TEST(test_HigherOrderFunction);
  RUN_TEST(test_RecursiveFunction);

  // variable definitions and scope
  RUN_TEST(test_define);
  RUN_TEST(test_LetBinding);
  RUN_TEST(test_UndefinedVariable);

  // list operations
  RUN_TEST(test_ConstructList);
  RUN_TEST(test_CarOperation);
  RUN_TEST(test_CdrOperation);

  RUN_TEST(test_ComplexExpression);

  RUN_TEST(test_TypeMismatch);

  RUN_TEST(test_CreateVector);
  RUN_TEST(test_VectorRef);
  RUN_TEST(test_VectorSet);
  RUN_TEST(test_VectorLength);

  RUN_TEST(test_Quote);
  RUN_TEST(test_QuasiquoteSimple);
  RUN_TEST(test_QuasiquoteWithUnquote);
  RUN_TEST(test_NestedQuasiquoteWithUnquote);
  RUN_TEST(test_UnquoteWithoutQuasiquote);

  return UNITY_END();
}
