#include <stdio.h>

#include "coroutines.h"

static struct coroutine main_thread;
static struct coroutine other_thread;

static char test_stack[8192];

static void coroutine_func(void)
{
	unsigned long r;

	r = (unsigned long)run_coroutine(&other_thread, &main_thread, (void *)5);
	printf("coroutine got %ld\n", r);
	throw 73;
}

int
main()
{
	unsigned long r;

	initialise_coroutine(&main_thread, "main thread");
	make_coroutine(&other_thread, "test thread",
		       test_stack, sizeof(test_stack),
		       (void *)coroutine_func, 0);

	r = (unsigned long)run_coroutine(&main_thread, &other_thread, (void *)52);
	printf("main thread got %ld\n", r);
	try {
	  r = (unsigned long)run_coroutine(&main_thread, &other_thread, (void *)53);
	} catch (int x) {
	  printf("Main thread caught exception %d\n", x);
	}
	printf("main thread got %ld and is exitting\n", r);
	return 0;
}
