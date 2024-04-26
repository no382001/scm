make:
	g++ -g tinylisp.c
test:
	g++ -g func_test.cpp -lgtest -lgtest_main -pthread
clean:
	rm *.o *.out