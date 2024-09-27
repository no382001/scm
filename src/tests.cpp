#include <fstream>
#include <gtest/gtest.h>
#include <thread>

#define FUNC_TEST
extern "C" {
#include "tinyscheme.c"
};

parsing_ctx test_ctx = {
    .file = NULL, .buffer = {0}, .buf_pos = 0, .buf_end = 0, .see = ' '};

struct LispTest : public ::testing::Test {

  void SetUp() override {
    g_err_state.type = NONE;
    g_err_state.proc = 0;
    g_err_state.box = 0;

    nil = box(NIL, 0);
    err = atom("ERR");
    tru = atom("#t");
    env = pair(tru, tru, nil);

    for (int i = 0; prim[i].s; ++i) {
      env = pair(atom(prim[i].s), box(PRIM, i), env);
    }

    curr_ctx = &test_ctx;
  }

  void TearDown() override {
    hp = 0;
    sp = N;
  }

  L eval_string(const char *input) {
    if (setjmp(jb) == 1) {
      return 1; // FIX: im not sure this is good here
    }

    size_t input_len = strlen(input);

    strncpy(curr_ctx->buffer, input, input_len);
    curr_ctx->buf_pos = 0;
    curr_ctx->buf_end = input_len;
    curr_ctx->see = curr_ctx->buffer[curr_ctx->buf_pos++];

    return eval(Read(), env);
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
      L car_val = car(x);
      L cdr_val = cdr(x);
      char car_str[128], cdr_str[128];

      lisp_expression_to_string(car_val, car_str, sizeof(car_str));
      lisp_expression_to_string(cdr_val, cdr_str, sizeof(cdr_str));

      sprintf(debug_output, "comparing CONS: (%s . %s) with %s", car_str,
              cdr_str, str);
      printf("%s\n", debug_output);
      char cons_repr[256];
      snprintf(cons_repr, sizeof(cons_repr), "(%s . %s)", car_str, cdr_str);
      return strcmp(cons_repr, str) == 0;
    } else if (T(x) == VECTOR) {
      I start = ord(x);       // Starting index of vector
      I size = cell[--start]; // First item in vector block is the size

      char vec_str[1024] =
          "#("; // Temporary buffer to build vector representation

      for (I i = 0; i < size; ++i) {
        char elem_str[128];
        lisp_expression_to_string(cell[start - 1 - i], elem_str,
                                  sizeof(elem_str));

        strcat(vec_str, elem_str);
        if (i < size - 1) {
          strcat(vec_str, " "); // Add space between elements
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
};
// positive tests

TEST_F(LispTest, AddSimple) {
  EXPECT_EQ(eval_string("(+ 1 1)\n"), 2);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, AddMultiple) {
  EXPECT_EQ(eval_string("(+ 1 2 3)\n"), 6);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, SubtractSimple) {
  EXPECT_EQ(eval_string("(- 5 3)\n"), 2);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, MultiplySimple) {
  EXPECT_EQ(eval_string("(* 2 3)\n"), 6);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, DivideSimple) {
  EXPECT_EQ(eval_string("(/ 6 2)\n"), 3);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, NestedOperations) {
  EXPECT_EQ(eval_string("(+ (* 2 3) (- 5 2))\n"), 9);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, LogicalTrue) {
  auto r = eval_string("(eq? 1 1)\n");
  EXPECT_EQ(match_variable(r, "#t"),
            true); // assuming '#t' is defined in your environment setup
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, LogicalFalse) {
  auto r = eval_string("(eq? 1 2)\n");
  EXPECT_EQ(match_variable(r, "()"),
            true); // assuming '()' is defined in your environment setup
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, ConstructList) {
  auto r = eval_string("(cons 1 2)\n");
  EXPECT_EQ(match_variable(r, "(1 . 2)"), true);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, CarOperation) {
  EXPECT_EQ(eval_string("(car (cons 1 2))\n"), 1);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, CdrOperation) {
  EXPECT_EQ(eval_string("(cdr (cons 1 2))\n"), 2);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, IfTrue) {
  EXPECT_EQ(eval_string("(if '(1) 1 2)\n"), 1);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, IfFalse) {
  EXPECT_EQ(eval_string("(if () 1 2)\n"), 2);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, DefineAndUseFunction) {
  eval_string("(define square (lambda (x) (* x x)))\n");
  EXPECT_EQ(eval_string("(square 3)\n"), 9);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, LetBinding) {
  EXPECT_EQ(eval_string("(let* (x 5) (y 3) (+ x y))\n"), 8);
  EXPECT_EQ(g_err_state.type, NONE);
}

// negative tests

TEST_F(LispTest, UndefinedVariable) {
  EXPECT_EQ(eval_string("(+ a 1)\n"), 1);
  EXPECT_EQ(g_err_state.type, ASSOC_VALUE_N_FOUND);
}

TEST_F(LispTest, TypeMismatch) { // not implemented
  EXPECT_EQ(eval_string("(+ 'a 1)\n"), err);
  EXPECT_EQ(eval_string("(+ (quote a) 1)\n"), err);
}

TEST_F(LispTest, Quote1) {
  auto res = eval_string("(quote a)\n");
  EXPECT_EQ(match_variable(res, "a"), true);
  EXPECT_EQ(g_err_state.type, NONE);

  res = eval_string("'b\n");
  EXPECT_EQ(match_variable(res, "b"), true);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, ComplexExpression) {
  EXPECT_EQ(eval_string("(* (+ 2 3) (- 5 2))\n"), 15);
}

TEST_F(LispTest, RecursiveFunction) {
  eval_string(
      "(define fact (lambda (n) (if (eq? n 1) 1 (* n (fact (- n 1))))))\n");
  EXPECT_EQ(eval_string("(fact 5)\n"), 120);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, DefineWoLambda) {
  ASSERT_EQ(
      eval_string(
          "(define (fib n) (if (< n 3) 1 (+ (fib (- n 1)) (fib (- n 2)))))\n"),
      1);
  ASSERT_EQ(g_err_state.type, FUNCTION_DEF_IS_NOT_LAMBDA);
}

TEST_F(LispTest, HigherOrderFunction) {
  eval_string("(define apply-func (lambda (f x) (f x)))\n");
  eval_string("(define inc (lambda (x) (+ x 1)))\n");
  EXPECT_EQ(eval_string("(apply-func inc 5)\n"), 6);
}

TEST_F(LispTest, PerformanceStressTest) {
  // This is a simplistic example; real tests should measure time and check for
  // performance regressions
  EXPECT_EQ(eval_string("(+ 1000000 1000000)\n"), 2000000);
}

/** /
TEST_F(LispTest, FileReadOperation) {
    // mock file read operation or ensure the environment is correctly set up
for I/O std::ofstream file("test.txt"); file << "(define x 42)\n"; file.close();
    EXPECT_EQ(eval_string("(load 'test.txt')\n"), 42);  // assuming 'load' is a
function that can read from files
}
*/

TEST_F(LispTest, Eval) { EXPECT_EQ(eval_string("(eval 42)\n"), 42); }

TEST_F(LispTest, Quote) {
  auto r = eval_string("(quote (1 2 3))\n");
  EXPECT_EQ(match_variable(r, "(1 . (2 . (3 . ())))"), true);
}

TEST_F(LispTest, Car) { EXPECT_EQ(eval_string("(car (cons 1 2))\n"), 1); }

TEST_F(LispTest, Cdr) { EXPECT_EQ(eval_string("(cdr (cons 1 2))\n"), 2); }

TEST_F(LispTest, Add) { EXPECT_EQ(eval_string("(+ 1 2 3 4)\n"), 10); }

TEST_F(LispTest, Subtract) { EXPECT_EQ(eval_string("(- 10 2 3)\n"), 5); }

TEST_F(LispTest, Multiply) { EXPECT_EQ(eval_string("(* 2 3 4)\n"), 24); }

TEST_F(LispTest, Divide) { EXPECT_EQ(eval_string("(/ 24 3 2)\n"), 4); }

TEST_F(LispTest, Integer) { EXPECT_EQ(eval_string("(int 3.75)\n"), 3); }

TEST_F(LispTest, LessThan) {
  auto r = eval_string("(< 3 4)\n");
  EXPECT_EQ(match_variable(r, "#t"), true);
}

TEST_F(LispTest, Or) {
  auto r = eval_string("(or () #t)\n");
  EXPECT_EQ(match_variable(r, "#t"), true);

  r = eval_string("(or () ())\n");
  EXPECT_EQ(match_variable(r, "()"), true);
}

TEST_F(LispTest, And) {
  auto r = eval_string("(and () ())\n");
  EXPECT_EQ(match_variable(r, "()"), true);

  r = eval_string("(and 'a 'a)\n");
  EXPECT_EQ(match_variable(r, "a"), true);
}

TEST_F(LispTest, Not) {
  auto r = eval_string("(not ())\n");
  EXPECT_EQ(match_variable(r, "#t"), true);
}

TEST_F(LispTest, Cond) {
  EXPECT_EQ(eval_string("(cond ((eq? 1 2) 3) ((eq? 2 2) 4))\n"), 4);
}

TEST_F(LispTest, If) { EXPECT_EQ(eval_string("(if (eq? 1 1) 42 100)\n"), 42); }

TEST_F(LispTest, LetStar) {
  EXPECT_EQ(eval_string("(let* (a 1) (b 2) (+ a b))\n"), 3);
}

TEST_F(LispTest, Lambda) {
  eval_string("(define square (lambda (x) (* x x)))\n");
  EXPECT_EQ(eval_string("(square 3)\n"), 9);
}

TEST_F(LispTest, Define) {
  eval_string("(define x 42)\n");
  EXPECT_EQ(eval_string("x\n"), 42);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST_F(LispTest, FBasicAddition) {
  ASSERT_NEAR(eval_string("(+ 0.1 0.2)\n"), 0.3, 0.0001);
}

TEST_F(LispTest, FineGrainedSubtraction) {
  EXPECT_EQ(eval_string("(- 0.30000000000000004 0.2)\n"), 0.10000000000000004);
}

TEST_F(LispTest, LargeNumberMultiplication) {
  EXPECT_EQ(eval_string("(* 100000000 100000000)\n"), 10000000000000000);
}

TEST_F(LispTest, DivisionBySmallNumber) {
  EXPECT_EQ(eval_string("(/ 1 0.0000001)\n"), 10000000);
}

TEST_F(LispTest, ChainedArithmetic) {
  ASSERT_NEAR(eval_string("(+ (* 2.5234 5.33) (- 10.003242301 3.6))\n"), 19.853,
              0.0001);
}

TEST_F(LispTest, IntegerOverflow) {
  EXPECT_EQ(eval_string("(+ 2147483647 1)\n"), 2147483648);
}

TEST_F(LispTest, FloatingPointUnderflow) {
  EXPECT_EQ(eval_string("(* 1e-308 1e-308)\n"), 0);
}

TEST_F(LispTest, ComplexFtpDebuf_DependsOnSTLFiles) {
  // make sure that sin is defined, it uses defun now
  eval_string("(load 'stl/math.scm)");

  ASSERT_NEAR(eval_string("(sin 1.0)"), 0.8414709848, 0.00001);
}

TEST_F(LispTest, CreateVector) {
  auto r = eval_string("(vector 1 2 3 4)\n");
  EXPECT_EQ(match_variable(r, "#(1 2 3 4)"), true);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, VectorRef) {
  eval_string("(define v (vector 1 2 3 4))\n");
  EXPECT_EQ(eval_string("(vector-ref v 0)\n"), 1);
  EXPECT_EQ(eval_string("(vector-ref v 1)\n"), 2);
  EXPECT_EQ(eval_string("(vector-ref v 2)\n"), 3);
  EXPECT_EQ(eval_string("(vector-ref v 3)\n"), 4);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, VectorSet) {
  eval_string("(define v (vector 1 2 3 4))\n");
  eval_string("(vector-set! v 1 42)\n");
  EXPECT_EQ(eval_string("(vector-ref v 1)\n"), 42);

  eval_string("(vector-set! v 0 100)\n");
  EXPECT_EQ(eval_string("(vector-ref v 0)\n"), 100);
  EXPECT_EQ(g_err_state.type, NONE);
}

TEST_F(LispTest, VectorLength) {
  eval_string("(define v (vector 1 2 3 4))\n");
  EXPECT_EQ(eval_string("(vector-length v)\n"), 4);
  EXPECT_EQ(g_err_state.type, NONE);
}