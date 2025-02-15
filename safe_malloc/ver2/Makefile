all:
	gcc -Wall -g -o main main.c safe_malloc.c

build_test_perf:
	gcc -g -o test_perf test_perf.c safe_malloc.c

test_perf: build_test_perf
	./test_perf

test_verific:
	gcc -Wall -g -o test_verification test_verification.c safe_malloc.c

run:
	./main.exe

clean:
	del main.exe