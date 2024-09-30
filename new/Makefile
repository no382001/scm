CC=gcc
CXX=g++
CFLAGS=-g -std=gnu99 -Wall -Werror -pedantic -O0
CXXFLAGS=-g
LDFLAGS=-lgtest -lgtest_main -pthread
LCOVFLAGS=-fprofile-arcs -ftest-coverage 
RM=rm -f

all: tinyscheme

tinyscheme: format
	$(CC) $(CFLAGS) parser.c print.c main.c

clean:
	$(RM) *.o *.out

format:
	find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) -exec clang-format -i {} \;
