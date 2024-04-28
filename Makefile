make:
	python3 gen_enum_map.py && gcc -g tinylisp.c
test:
	python3 gen_enum_map.py && g++ -g func_test.cpp -lgtest -lgtest_main -pthread
clean:
	rm *.o *.out error_map.h