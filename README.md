# thread-pool
A thread pool implemented in C with different options for storing waiting tasks

This implementation allows for the creation of a thread pool. The threads in the thread pool idle until a task is added into them. When tasks are available the threads execute them until no more tasks remain. Then the threads return to their idle state until more work is added or the thread pool is destroyed.

This implementation supports five different options for storing waiting tasks while waiting for an available thread. The tasks can be stored as a Binary Heap, a Binomial Heap, a Fibonacci Heap, a First In First Out Queue, or a Last In Last Out Queue. The Binary, Binomial, and Fibonacci Heap options require a comparision function that is used to determine the relative priority between two tasks. The tasks in FIFO and LIFO Queues are executed based on time entered into the queue.

========================================================================

Quick overview of how to setup this implementation:

Download queues.h, structs.h, thread_pool.h, and thread_pool.c to a working directory.


Include thread_pool.h  and pthread.h headers in my_program.c

#include "thread_pool.h"
#include <pthread.h>


Compile with thread_pool.c and flag -pthread

$ gcc -pthread thread_pool.c my_program.c 


========================================================================


Examples of usage of functions provided:

struct thread_pool* create_pool(
		int number_threads,
		int mode, 
		int (*function)(const void* p1, const void* p2));

Creates a thread pool. The first parameter is the number of threads that service the pool. The second parameter specifies how the waiting tasks will be stored before execution.

  1 = Binary Heap
  2 = Binomial Heap
  3 = Fibonacci Heap
  4 = First In First Out Queue
  5 = Last In First Out Queue

Any other input defaults to a Binary Heap.
The final parameter is a pointer to a comparision function that can be used to determine which of two tasks has a higher priority. Note that this parameter should be set to NULL if tasks are stored in a FIFO or LIFO Queue. 
Consider a case in which each task consists of sorting an array of a variable length using an insert sort function. It is desired that the arrays with the greatest number of elements be sorted first. 

struct insert_sort_info{

  int* v;
  int left;
  int right;
};

int compare(const void* p1, const void* p2){

  struct insert_sort_info* s1 = (struct insert_sort_info*)p1;
  struct insert_sort_info* s2 = (struct insert_sort_info*)p2;

  int len1 = s1->right - s1->left;
  int len2 = s2->right - s2->left;
  
  return len1-len2;
}

If 'compare' returns greater than 0 than the task with struct insert_sort_info1 is considered to be of greater priority than the task with insert_sort_info2. If 'compare' returns less than 0 the second parameter has greater priority than the first.


	struct thread_pool* pool = create_pool(4, 3, compare);

Creates a thread pool with 4 threads backed by a Binary_Heap and priority is set by function 'compare'.

	struct thread_pool* pool = create_pool(4, 5, NULL);

Creates a thread pool with 4 threads backed by a LIFO Queue. No comparison function is need.

------------------------------------------------------------------------

add_threads(2, pool);

Adds two more threads to an existing thread pool.

------------------------------------------------------------------------

void add_task(struct thread_pool* pool, 
		void (*function)(void* arg),
		void* arg);

Adds a task into the thread pool: The add_task function takes a reference to the pool, a pointer to a function to be performed, and void* argument. This is typically a pointer to a structure that holds the arguments of the function.

For example: given a function insert_sort that is typically declared:

void insert_sort(int* v, left, right);

modify declaration to:

void insert_sort(struct info* arguments);

with

struct info{

       int* v;
       int left;
       int right;
};

Then add to queue with:

add_task(pool, insert_sort, (void*)(arguments));

------------------------------------------------------------------------

Destroy the thread pool when no longer needed. There are two options for this. destroy_pool_immediately destroys the thread pool even if there are tasks still in the queue. destroy_pool_when_idle allows the thread pool to continue to service the queue until queue is empty. Then the threads are terminated.

destroy_pool_immediately(pool);
destroy_pool_when_idle(pool);

