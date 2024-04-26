#include <gtest/gtest.h>
#include <thread>
#include <fstream>

#define FUNC_TEST
#include "tinylisp.cpp"

class LispTest : public ::testing::Test {
protected:
    void SetUp() override {

        nil = box(NIL, 0);
        err = atom("ERR");
        tru = atom("#t");
        env = pair(tru, tru, nil);
        for (int i = 0; prim[i].s; ++i) {
            env = pair(atom(prim[i].s), box(PRIM, i), env);
        }
    }

    void TearDown() override {
        hp = 0;
        sp = N;

        freopen("/dev/tty", "w", stdout); // Reset stdout
        freopen("/dev/tty", "w", stderr); // Reset stderr
        freopen("/dev/tty", "r", stdin);  // Reset stdin
        std::cout.clear();  // Clear any errors
        std::cerr.clear();  // Clear any errors
        std::cin.clear();   // Clear any errors
    }

    L eval_string(const char* code) {
        int backup_stdin = dup(STDIN_FILENO); // backup stdin
        int fds[2];
        if (pipe(fds) != 0) {
            perror("failed to create pipe");
            throw;
        }

        if (write(fds[1], code, strlen(code)) == -1){
            throw std::runtime_error("write failed");
        }

        // close write end of the pipe after writing
        if (close(fds[1]) == -1){
            throw std::runtime_error("close failed");
        }

        // replace stdin with the read end of the pipe
        if (dup2(fds[0], STDIN_FILENO) == -1){
            throw std::runtime_error("dup2 failed");
        } 

        L result = eval(Read(), env);

        // restore the original stdin
        if (dup2(backup_stdin, STDIN_FILENO) == -1){
            throw std::runtime_error("dup2 failed");
        } 
        
        // close the read end of the pipe
        if (close(fds[0]) == -1){
            throw std::runtime_error("close failed");
        }

        // close the backup descriptor
        if (close(backup_stdin) == -1){
            throw std::runtime_error("close failed");
        }

        return result;
    }
};

// positive tests

TEST_F(LispTest, AddSimple) {
    std::cerr << "Starting test X, stdin and stdout should be ok here." << std::endl;
    EXPECT_EQ(eval_string("(+ 1 1)"), 2);
}

TEST_F(LispTest, AddMultiple) {
    std::cerr << "Starting test X, stdin and stdout should be ok here." << std::endl;
    EXPECT_EQ(eval_string("(+ 1 2 3)"), 6);
}

TEST_F(LispTest, SubtractSimple) {
    EXPECT_EQ(eval_string("(- 5 3)"), 2);
}

TEST_F(LispTest, MultiplySimple) {
    EXPECT_EQ(eval_string("(* 2 3)"), 6);
}

TEST_F(LispTest, DivideSimple) {
    EXPECT_EQ(eval_string("(/ 6 2)"), 3);
}

TEST_F(LispTest, NestedOperations) {
    EXPECT_EQ(eval_string("(+ (* 2 3) (- 5 2))"), 9);
}

TEST_F(LispTest, LogicalTrue) {
    EXPECT_EQ(eval_string("(eq? 1 1)"), tru); // assuming 'tru' is defined in your environment setup
}

TEST_F(LispTest, LogicalFalse) {
    EXPECT_EQ(eval_string("(eq? 1 0)"), nil); // assuming 'nil' is defined as false
}

TEST_F(LispTest, ConstructList) {
    EXPECT_EQ(eval_string("(cons 1 2)"), cons(1, 2)); // this assumes that 'cons' and number evaluation works correctly
}

TEST_F(LispTest, CarOperation) {
    EXPECT_EQ(eval_string("(car (cons 1 2))"), 1);
}

TEST_F(LispTest, CdrOperation) {
    EXPECT_EQ(eval_string("(cdr (cons 1 2))"), 2);
}

TEST_F(LispTest, IfTrue) {
    EXPECT_EQ(eval_string("(if #t 1 2)"), 1);
}

TEST_F(LispTest, IfFalse) {
    EXPECT_EQ(eval_string("(if #f 1 2)"), 2);
}

TEST_F(LispTest, DefineAndUseFunction) {
    eval_string("(define square (lambda (x) (* x x)))");
    EXPECT_EQ(eval_string("(square 3)"), 9);
}

TEST_F(LispTest, LetBinding) {
    EXPECT_EQ(eval_string("(let ((x 5) (y 3)) (+ x y))"), 8);
}

// negative tests

TEST_F(LispTest, UndefinedVariable) {
    EXPECT_EQ(eval_string("(+ a 1)"), err);
}

TEST_F(LispTest, TypeMismatch) {
    EXPECT_EQ(eval_string("(+ 'a 1)"), err);
}

TEST_F(LispTest, MemoryOverflow) {
    std::string largeInput = "(cons 1 2)";
    largeInput.reserve(1024 * 1024);  // artificially large input to test memory handling
    EXPECT_EQ(eval_string(largeInput.c_str()), err);
}

TEST_F(LispTest, ComplexExpression) {
    EXPECT_EQ(eval_string("(* (+ 2 3) (- 5 2))"), 15);
}

TEST_F(LispTest, RecursiveFunction) {
    eval_string("(define fact (lambda (n) (if (eq? n 1) 1 (* n (fact (- n 1))))))");
    EXPECT_EQ(eval_string("(fact 5)"), 120);
}

TEST_F(LispTest, HigherOrderFunction) {
    eval_string("(define apply-func (lambda (f x) (f x)))");
    eval_string("(define inc (lambda (x) (+ x 1)))");
    EXPECT_EQ(eval_string("(apply-func inc 5)"), 6);
}

TEST_F(LispTest, ThreadSafety) { // im not sure..
    std::thread t1([&](){ eval_string("(define x 10)"); });
    std::thread t2([&](){ eval_string("(define y 20)"); });
    t1.join();
    t2.join();
    EXPECT_EQ(eval_string("(+ x y)"), 30);
}

TEST_F(LispTest, PerformanceStressTest) {
    // This is a simplistic example; real tests should measure time and check for performance regressions
    EXPECT_EQ(eval_string("(+ 1000000 1000000)"), 2000000);
}

TEST_F(LispTest, FileReadOperation) {
    // mock file read operation or ensure the environment is correctly set up for I/O
    std::ofstream file("test.txt");
    file << "(define x 42)";
    file.close();
    EXPECT_EQ(eval_string("(load 'test.txt')"), 42);  // assuming 'load' is a function that can read from files
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}