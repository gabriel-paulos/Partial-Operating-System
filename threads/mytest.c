#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "thread.h"
#include "test_thread.h"

static void hello(char *msg);

int main(){

  Tid ret;
  Tid ret2;
  Tid ret3;

  void* scavage = "hello from first thread";
  
  ret = thread_create((void (*)(void*)) hello, scavage);
  
  ret2 = thread_create((void (*)(void*)) hello, scavage);

  ret3 = thread_create((void (*)(void*)) hello, scavage);

  printf("switching threads\n");

  printf("thread id: %d\n", thread_id());

  ret = thread_yield(ret2);

  printf("thread id: %d\n", thread_id());

  printf("from thread 2 to thread 3");

  printf("thread id: %d\n", thread_id());

  ret = thread_yield(ret3);

  printf("thread id: %d\n", thread_id());

  printf("%d", ret);
  
  return 0;
}

static void hello(char *msg)
{
	Tid ret;
	char str[20];

	printf("message: %s\n", msg);
	ret = thread_yield(THREAD_SELF);
	assert(thread_ret_ok(ret));
	printf("thread returns from  first yield\n");

	/* we cast ret to a float because that helps to check
	 * whether the stack alignment of the frame pointer is correct */
	sprintf(str, "%3.0f\n", (float)ret);

	ret = thread_yield(THREAD_SELF);
	assert(thread_ret_ok(ret));
	printf("thread returns from second yield\n");

	while (1) {
		thread_yield(THREAD_ANY);
	}

}
