CFLAGS=-Wall -O3
CXXFLAGS=-Wall

all: libcoroutines.a test_results

install: coroutines.h libcoroutines.a
	install coroutines.h /usr/local/include
	install libcoroutines.a /usr/local/lib

libcoroutines.a: coroutines.o
	ar rcs $@ $<

coroutines.o: coroutines.c coroutines.h
	gcc $(CFLAGS) -c coroutines.c

test_results: test1.result test2.result test3.result
	cat $^ > $@ && if [ -s test_results ]; then echo "test failed: "; cat $@; false; else echo "tests passed"; fi

%.result: %.ref %.cur
	diff -u $^ > $@

%.cur: %
	./$< > $@

test1.o: coroutines.h

test1: test1.o libcoroutines.a 
	gcc $^ -o $@

test2.o: coroutines.h

test2: test2.o libcoroutines.a 
	gcc -lstdc++ $^ -o $@

test2.cur: test2
	./test2 > test2.cur 2>&1 || true

test3.o: coroutines.h

test3: test3.o libcoroutines.a 
	gcc -lstdc++ $^ -o $@


clean:
	rm -f *.a *.o test_results *.result *.cur *~ core test1 test2 test3

