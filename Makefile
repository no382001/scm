CC=gcc
CFLAGS=-g -std=gnu99 -Wall -Werror -pedantic -O0 -Wno-error=unused-variable -Wno-error=pedantic -Wno-error=unused-function
RM=rm -f
SRC=src/parser.c src/print.c src/main.c src/primitives.c src/interpreter.c src/builder.c

all: tinyscm

tinyscm: gen
	$(CC) $(CFLAGS) $(SRC)

clean:
	$(RM) *.o *.out

format:
	find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) -exec clang-format -i {} \;

tests:
	$(CC) -DUNITY_TEST $(CFLAGS) -Wno-error=format-overflow= $(SRC) src/tests.c unity/src/unity.c

gen: 
	python3 util/gen_enum_map.py