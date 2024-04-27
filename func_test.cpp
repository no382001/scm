#include <gtest/gtest.h>
#include <thread>
#include <fstream>

#define EXPECT_THROW_VALUE(f,val)\
try {\
    int r = f;\
    FAIL() << "Expected exception, got: " << r << " or 0x" << std::hex << r;\
} catch (decltype(val) exception) {\
    EXPECT_EQ(exception, val);\
} catch (...) {\
    FAIL() << "Expected int exception, but caught different type";\
}


#define FUNC_TEST
extern "C" {
    #include "tinylisp.c"
};

int orig_stdin;
int input_pipe[2];
ERROR_CODE i; // for setjmp

struct LispTest : public ::testing::Test {

    void SetUp() override {
        nil = box(NIL, 0);
        //err = atom("ERR");

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
        if ((i = (ERROR_CODE)setjmp(jb)) != 0) throw i;

        write(input_pipe[1], input, strlen(input));
        
        return eval(Read(), env);
    }

    bool match_variable(int x, const char* str){
        auto var = A+ord(x);
        if (strcmp(var,str)){
            return true;
        } else {
            return false;
        }
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
    EXPECT_THROW_VALUE(eval_string("(+ a 1)\n"),ASSOC_VALUE_N_FOUND);
}

TEST_F(LispTest, TypeMismatch) {
    trace = 1;
    EXPECT_THROW_VALUE(eval_string("(+ 'a 1)\n"),TYPE_MISMATCH);
}

TEST_F(LispTest, Quote1) {
    auto res = eval_string("(quote a)\n");
    EXPECT_EQ(match_variable(res,"a"),true);

    res = eval_string("('a)\n");
    EXPECT_EQ(match_variable(res,"a"),true);
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

TEST_F(LispTest, Eval) {
    EXPECT_EQ(eval_string("(eval 42)\n"), 42);
}

TEST_F(LispTest, Quote) {
    EXPECT_EQ(eval_string("(quote (1 2 3))\n"), cons(cons(1, cons(2, cons(3, nil))), nil));
}

TEST_F(LispTest, Cons) {
    EXPECT_EQ(eval_string("(cons 1 2)\n"), cons(1, 2));
}

TEST_F(LispTest, Car) {
    EXPECT_EQ(eval_string("(car (cons 1 2))\n"), 1);
}

TEST_F(LispTest, Cdr) {
    EXPECT_EQ(eval_string("(cdr (cons 1 2))\n"), 2);
}

TEST_F(LispTest, Add) {
    EXPECT_EQ(eval_string("(+ 1 2 3 4)\n"), 10);
}

TEST_F(LispTest, Subtract) {
    EXPECT_EQ(eval_string("(- 10 2 3)\n"), 5);
}

TEST_F(LispTest, Multiply) {
    EXPECT_EQ(eval_string("(* 2 3 4)\n"), 24);
}

TEST_F(LispTest, Divide) {
    EXPECT_EQ(eval_string("(/ 24 3 2)\n"), 4);
}

TEST_F(LispTest, Integer) {
    EXPECT_EQ(eval_string("(int 3.75)\n"), 3);
}

TEST_F(LispTest, LessThan) {
    EXPECT_EQ(eval_string("(< 3 4)\n"), tru);
}

TEST_F(LispTest, Equal) {
    EXPECT_EQ(eval_string("(eq? 3 3)\n"), tru);
}

TEST_F(LispTest, Or) {
    EXPECT_EQ(eval_string("(or () #t)\n"), tru);
}

TEST_F(LispTest, And) {
    EXPECT_EQ(eval_string("(and #t #t)\n"), tru);
}

TEST_F(LispTest, Not) {
    EXPECT_EQ(eval_string("(not ())\n"), tru);
}

TEST_F(LispTest, Cond) {
    EXPECT_EQ(eval_string("(cond ((eq? 1 2) 3) ((eq? 2 2) 4))\n"), 4);
}

TEST_F(LispTest, If) {
    EXPECT_EQ(eval_string("(if (eq? 1 1) 42 100)\n"), 42);
}

TEST_F(LispTest, LetStar) {
    EXPECT_EQ(eval_string("(let* ((a 1) (b 2)) (+ a b))\n"), 3);
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