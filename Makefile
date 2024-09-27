CC=gcc
CXX=g++
CFLAGS=-g -std=gnu99 -Wall -Werror -pedantic -O0
CXXFLAGS=-g
LDFLAGS=-lgtest -lgtest_main -pthread
LCOVFLAGS=-fprofile-arcs -ftest-coverage 
RM=rm -f

all: tinyscheme

tinyscheme: gen_enum
	find . -name "*.[ch]" -exec clang-format -i {} \; && \
	$(CC) $(CFLAGS) tinyscheme.c -o tinyscheme

test: gen_enum
	$(CXX) $(CXXFLAGS) tests/func_test.cpp $(LDFLAGS) -o test && ./test

coverage: gen_enum
	$(CXX) $(CXXFLAGS) $(LCOVFLAGS) func_test.cpp $(LDFLAGS) -lgcov -o test_coverage \
	&& lcov -c -o coverage.info -d . && ./test_coverage && genhtml coverage.info -o cov_report

gen_enum:
	python3 util/gen_enum_map.py

clean:
	$(RM) *.o *.out error_map.h tinyscheme test* coverage.info func_test.gcda func_test.gcno

help:
	@echo "Usage:"
	@echo "  make [target]"
	@echo "Targets:"
	@echo "  all      - builds the main application"
	@echo "  test     - builds and runs the tests"
	@echo "  gen_enum - Generates enum mappings"
	@echo "  clean    - removes all generated files"
	@echo "  coverage - generates code coverage report (not implemented)"
