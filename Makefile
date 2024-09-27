CC=gcc
CXX=g++
CFLAGS=-g -std=gnu99 -Wall -Werror -pedantic -O0
CXXFLAGS=-g
LDFLAGS=-lgtest -lgtest_main -pthread
LCOVFLAGS=-fprofile-arcs -ftest-coverage 
RM=rm -f

all: tinyscheme

tinyscheme: gen format
	$(CC) $(CFLAGS) tinyscheme.c -o tinyscheme

test: gen
	$(CXX) $(CXXFLAGS) tests/func_test.cpp $(LDFLAGS) -o test && ./test

coverage: gen
	$(CXX) $(CXXFLAGS) $(LCOVFLAGS) func_test.cpp $(LDFLAGS) -lgcov -o test_coverage \
	&& lcov -c -o coverage.info -d . && ./test_coverage && genhtml coverage.info -o cov_report

gen:
	python3 util/gen_enum_map.py && \
	python3 util/gen_fwd_decl.py

clean:
	$(RM) *.o *.out error_map.h tinyscheme test* coverage.info func_test.gcda func_test.gcno

format:
	find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) -exec clang-format -i {} \;

help:
	@echo "Usage:"
	@echo "  make [target]"
	@echo "Targets:"
	@echo "  all      - builds the main application"
	@echo "  test     - builds and runs the tests"
	@echo "  gen_enum - Generates enum mappings"
	@echo "  clean    - removes all generated files"
	@echo "  coverage - generates code coverage report (not implemented)"
