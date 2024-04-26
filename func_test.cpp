#include <gtest/gtest.h>
#include <thread>
#include <fstream>

#define FUNC_TEST
#include "tinylisp.cpp"

int orig_stdin;
int input_pipe[2];
struct LispTest : public ::testing::Test {

    void SetUp() override {

        nil = box(NIL, 0);
        err = atom("ERR");
        tru = atom("#t");
        env = pair(tru, tru, nil);
        for (int i = 0; prim[i].s; ++i) {
            env = pair(atom(prim[i].s), box(PRIM, i), env);
        }

        orig_stdin = dup(STDIN_FILENO);

        // create a pipe to replace stdin
        ASSERT_EQ(pipe(input_pipe), 0);

        // replace stdin with the read end of the pipe
        dup2(input_pipe[0], STDIN_FILENO);

        // close the original read end of the pipe to avoid leaks
        close(input_pipe[0]);
    }

    void TearDown() override {
        hp = 0;
        sp = N;

        dup2(orig_stdin, STDIN_FILENO);
        close(orig_stdin);

        // close the write end of the pipe
        close(input_pipe[1]);
    }

    L eval_string(const char* input) {
        write(input_pipe[1], input, strlen(input));
        
        return eval(Read(), env);
    }
};

// positive tests

TEST_F(LispTest, AddSimple) {
    EXPECT_EQ(eval_string("(+ 1 1)\n"), 2);
}

TEST_F(LispTest, AddMultiple) {
    EXPECT_EQ(eval_string("(+ 1 2 3)\n"), 6);
}

TEST_F(LispTest, SubtractSimple) {
    EXPECT_EQ(eval_string("(- 5 3)\n"), 2);
}

TEST_F(LispTest, MultiplySimple) {
    EXPECT_EQ(eval_string("(* 2 3)\n"), 6);
}

TEST_F(LispTest, DivideSimple) {
    EXPECT_EQ(eval_string("(/ 6 2)\n"), 3);
}

TEST_F(LispTest, NestedOperations) {
    EXPECT_EQ(eval_string("(+ (* 2 3) (- 5 2))\n"), 9);
}

TEST_F(LispTest, LogicalTrue) {
    EXPECT_EQ(eval_string("(eq? 1 1)\n"), tru); // assuming 'tru' is defined in your environment setup
}

TEST_F(LispTest, LogicalFalse) {
    EXPECT_EQ(eval_string("(eq? 1 0)\n"), nil); // assuming 'nil' is defined as false
}

TEST_F(LispTest, ConstructList) {
    EXPECT_EQ(eval_string("(cons 1 2)\n"), cons(1, 2)); // this assumes that 'cons' and number evaluation works correctly
}

TEST_F(LispTest, CarOperation) {
    EXPECT_EQ(eval_string("(car (cons 1 2))\n"), 1);
}

TEST_F(LispTest, CdrOperation) {
    EXPECT_EQ(eval_string("(cdr (cons 1 2))\n"), 2);
}

TEST_F(LispTest, IfTrue) {
    EXPECT_EQ(eval_string("(if #t 1 2)\n"), 1);
}

TEST_F(LispTest, IfFalse) {
    EXPECT_EQ(eval_string("(if #f 1 2)\n"), 2);
}

TEST_F(LispTest, DefineAndUseFunction) {
    eval_string("(define square (lambda (x) (* x x)))\n");
    EXPECT_EQ(eval_string("(square 3)\n"), 9);
}

TEST_F(LispTest, LetBinding) {
    EXPECT_EQ(eval_string("(let ((x 5) (y 3)) (+ x y))\n"), 8);
}

// negative tests

TEST_F(LispTest, UndefinedVariable) {
    EXPECT_EQ(eval_string("(+ a 1)\n"), err);
}

TEST_F(LispTest, TypeMismatch) {
    EXPECT_EQ(eval_string("(+ 'a 1)\n"), err);
}
/** /
TEST_F(LispTest, MemoryOverflow) {
    std::string largeInput = "(cons 1 2)\n";
    largeInput.reserve(1024 * 1024);  // artificially large input to test memory handling
    EXPECT_EQ(eval_string(largeInput.c_str()), err);
}
*/
TEST_F(LispTest, ComplexExpression) {
    EXPECT_EQ(eval_string("(* (+ 2 3) (- 5 2))\n"), 15);
}

TEST_F(LispTest, RecursiveFunction) {
    eval_string("(define fact (lambda (n) (if (eq? n 1) 1 (* n (fact (- n 1))))))\n");
    EXPECT_EQ(eval_string("(fact 5)\n"), 120);
}

TEST_F(LispTest, HigherOrderFunction) {
    eval_string("(define apply-func (lambda (f x) (f x)))\n");
    eval_string("(define inc (lambda (x) (+ x 1)))\n");
    EXPECT_EQ(eval_string("(apply-func inc 5)\n"), 6);
}

TEST_F(LispTest, ThreadSafety) { // im not sure..
    std::thread t1([&](){ eval_string("(define x 10)\n"); });
    std::thread t2([&](){ eval_string("(define y 20)\n"); });
    t1.join();
    t2.join();
    EXPECT_EQ(eval_string("(+ x y)\n"), 30);
}

TEST_F(LispTest, PerformanceStressTest) {
    // This is a simplistic example; real tests should measure time and check for performance regressions
    EXPECT_EQ(eval_string("(+ 1000000 1000000)\n"), 2000000);
}

/** /
TEST_F(LispTest, FileReadOperation) {
    // mock file read operation or ensure the environment is correctly set up for I/O
    std::ofstream file("test.txt");
    file << "(define x 42)\n";
    file.close();
    EXPECT_EQ(eval_string("(load 'test.txt')\n"), 42);  // assuming 'load' is a function that can read from files
}
*/

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}