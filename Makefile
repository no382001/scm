CC=gcc
CFLAGS=-g -std=gnu99 -Wall -Werror -pedantic -O0 -Wno-error=unused-variable
RM=rm -f
SRC=src/parser.c src/print.c src/main.c src/primitives.c src/interpreter.c src/builder.c

all: tinyscm

tinyscm: format
	$(CC) $(CFLAGS) $(SRC)

clean:
	$(RM) *.o *.out

format:
	find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) -exec clang-format -i {} \;

tests:
	$(CC) -DUNITY_TEST $(CFLAGS) -Wno-error=format-overflow= $(SRC) src/tests.c unity/src/unity.c