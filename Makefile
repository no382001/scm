make:
	g++ -g tinylisp.cpp
test:
	g++ -g func_test.cpp -lgtest -lgtest_main -pthread
clean:
	rm *.o *.out