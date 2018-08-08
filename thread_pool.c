/* This c files holds the functions that allow for the creation of a
thread pool. The thread pool is seeded with as many threads as the
user desires. The threads service a queue of tasks feed into the
pool. See queue.h for more complete documentation on the usage of
the functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "thread_pool.h"
#include "structs.h"
#include "queues.h"


//Function Declarations-------------------------------------


struct thread_pool* create_pool(int number_threads, int mode);
void create_thread(struct thread_pool* pool);
void set_queue_mode(struct thread_pool* pool, int mode);
void add_threads(int number_to_add, struct thread_pool* pool);
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg);
struct task* pull_task(struct thread_pool* pool);
void* do_work(void* parameter);
void close_immediately(struct thread_pool* pool);
void close_when_idle(struct thread_pool* pool);
void destroy_pool_immediately(struct thread_pool* pool);
void destroy_pool_when_idle(struct thread_pool* pool);
void set_comparision(int (*function)(const void* p1, const void* p2), struct thread_pool* pool);

//Binomial Heap Functions-----------------------------------
struct task* binomial_pull_task(struct thread_pool* pool);
void binomial_push_task(struct task* to_add, struct thread_pool* pool);

//Binary Heap Functions-------------------------------------
struct task* binary_pull_task(struct thread_pool* pool);
void binary_push_task(struct task* to_add, struct thread_pool* pool);

//Fibonacci Heap Functions----------------------------------
struct task* fibonacci_pull_task(struct thread_pool* pool);
void fibonacci_push_task(struct task* to_add, struct thread_pool* pool);

//FIFO Functions---------------------------------------------
struct task* FIFO_pull_task(struct thread_pool* pool);
void FIFO_push_task(struct task* to_add, struct thread_pool* pool);

//LIFO Functions---------------------------------------------
struct task* LIFO_pull_task(struct thread_pool* pool);
void LIFO_push_task(struct task* to_add, struct thread_pool* pool);


//==================================================================



/*creates an instance of a thread pool seeded with number_threads.
---modify_pool is the mutex that allows only access to the pool by
      one of the created threads at a time. 
---signal_change is the broadcast that tells the threads that either
      a new task has been added to the queue or threads should be
      terminated.
---oldest task and newest task point to the each end of a doubly linked 
      list of tasks in the queue.
---FIFO (first in first out) is set to 1 by default. The threads will
      execute the first task added to the queue first. FIFO == 0
      will change the execution to LIFO (last in first out).
---kill_immediately flag tells the threads to terminate as soon as
      the execution of the current task is complete. It will also
      terminate any idle theads.
---kill_when_idle flags tells the threads to terminate when idle.
      This allows the threads to continue working on the queue until
      empty.
*/
struct thread_pool* create_pool(int number_threads, int mode){
  
  struct thread_pool* pool = malloc(sizeof(struct thread_pool));

  if(pool == NULL){
    printf("ERROR: %s\n", strerror(errno));
    return NULL;
  }

  set_queue_mode(pool, mode);
  
  pthread_mutex_init(&pool->modify_pool, NULL);
  pthread_cond_init(&pool->signal_change, NULL);
  
  pool->number_threads = number_threads;
  
  // pool->oldest_task = NULL;
  // pool->newest_task = NULL;

  pool->head = NULL;
  pool->tail = NULL;
  
  pool->num_tasks_in_queue = 0;
 
  pool->kill_immediately = 0;
  pool->kill_when_idle = 0;
  
  pool->thread_list = NULL;
  
  add_threads(number_threads, pool);
  
  return pool;
}

void set_queue_mode(struct thread_pool* pool, int mode){

  switch(mode){
  case 1:
    pool->push = binary_push_task;
    pool->pull = binary_pull_task;
    break;
    
  case 2:
    pool->push = binomial_push_task;
    pool->pull = binomial_pull_task;
    break;

  case 3:
    pool->push = fibonacci_push_task;
    pool->pull = fibonacci_pull_task;
    break;

  case 4:
    pool->push = FIFO_push_task;
    pool->pull = FIFO_pull_task;
    break;

  case 5:
    pool->push = LIFO_push_task;
    pool->pull = LIFO_pull_task;
    break;

  default:
    printf("ERROR: mode selection must be int between 1 and 5.\nDefault to Binary Heap");
    pool->push = binary_push_task;
    pool->pull = binary_pull_task;

    break;
  }
  
  return;
}
    

/*Creates a new thread and adds to thread_list maintained in the 
threadpool
 */
void create_thread(struct thread_pool* pool){

  struct thread_info* temp = malloc(sizeof(struct thread_info));

  if(temp == NULL){
    printf("ERROR: %s\n", strerror(errno));
  }
     
  temp->pool = pool;

  if(pthread_create(&temp->thread, NULL, do_work, temp) != 0){
    printf("ERROR: %s\n", strerror(errno));
  }

  temp->next = pool->thread_list;
  pool->thread_list = temp;

  return;
}

/*Adds number_to_add threads to the thread pool
 */
void add_threads(int number_to_add, struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: Parameter is not a valid thread_pool\n");
    return;
  }
  
  if(number_to_add < 0){
    printf("ERROR: Cannot add %d threads to thread pool\n", number_to_add);
    return;
  }
  
  pthread_mutex_lock(&pool->modify_pool);

  for(int i=0 ; i<number_to_add; i++){
    create_thread(pool);   
  }

  pool->number_threads = pool->number_threads + number_to_add;
  
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}
  
void add_task(struct thread_pool* pool, void (*function)(void* arg), void* arg){

  if(pool == NULL){
    printf("ERROR: First parameter is not a valid thread_pool\n");
    return;
  }
  
  struct task* new_task = malloc(sizeof(struct task));
  if(new_task == NULL){
    printf("ERROR: %s\n", strerror(errno));
  }
  
  new_task->function = function;
  new_task->arg = arg;
  /* 
  new_task->order = 0;
  new_task->sibling = NULL;
  new_task->parent = NULL;
  new_task->child = NULL;
  */
  pthread_mutex_lock(&pool->modify_pool);
  
  pool->num_tasks_in_queue++;

  pool->push(new_task, pool);
  
  //signal to thread pool that a new task is available
  //this will wake up an idling thread if one is available
  pthread_cond_broadcast(&pool->signal_change);
  pthread_mutex_unlock(&pool->modify_pool);
  
  return;
}

/*Return pointer to  oldest item in queue if FIFO == 1. Otherwise
return newest item in queue. Manipulate pointers to remove item from
queue.
 */
struct task* pull_task(struct thread_pool* pool){

  if(pool->num_tasks_in_queue == 0){
    
    return NULL;
  }

  else{

    struct task* to_do;
  
    to_do = pool->pull(pool);
    pool->num_tasks_in_queue--;
    
   
    return to_do;
  }
}

/*This is the thread where the work of the threads is accomplished.
It is infinite loop that can only be broken when either the 
kill_immediately or kill_when_idle flag is set. Otherwise the loop
look for a task in the queue and execute it. Failing this it will wait
until it receives a signal that a task is available of one of the kill
flags has been flipped.
*/ 
void* do_work(void* parameter){

  struct thread_info* a = (struct thread_info*)(parameter);
  struct task* to_do;

  struct thread_pool* pool = a->pool;
  
  while(1){

    pthread_mutex_lock(&pool->modify_pool);

    if(pool->kill_immediately == 1){
      pthread_mutex_unlock(&pool->modify_pool);
      return NULL;
    }

    //Put thread to sleep while waits for more work
    while(pool->num_tasks_in_queue == 0){

      if(pool->kill_when_idle == 1){

	pthread_mutex_unlock(&pool->modify_pool);
	return NULL;
      }

      pthread_cond_wait(&pool->signal_change, &pool->modify_pool);

       if(pool->kill_immediately == 1){

	 pthread_mutex_unlock(&pool->modify_pool);
	 return NULL;
       }
    }

    //At this point there must be a task available and the thread
    //owns the modify_pool mutex

    //Grab the new task
    to_do = pull_task(pool);

    pthread_mutex_unlock(&pool->modify_pool);

    //Call the function
    to_do->function(to_do->arg);

    free(to_do);
    to_do = NULL;

  }

  return NULL;
}

//Threads complete their tasks and then close
void close_immediately(struct thread_pool* pool){
    
  pthread_mutex_lock(&pool->modify_pool);

  pool->kill_immediately = 1;
	
  pthread_cond_broadcast(&pool->signal_change);

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}

//Flips kill_when_idle_flag and sends out signal
void close_when_idle(struct thread_pool* pool){

  pthread_mutex_lock(&pool->modify_pool);

  pool->kill_when_idle = 1;

  pthread_cond_broadcast(&pool->signal_change);

  pthread_mutex_unlock(&pool->modify_pool);

  return;
}

/*Waits for threads to be idle (queue empty and all tasks complete)
before closing
*/
void destroy_pool_when_idle(struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: parameter is not a valid thread_pool\n");
    return;
  }

  //Give the close signal to the working or idle threads
  close_when_idle(pool);
  
  //Free the linked list pointed to by pool->head_of_thread_info
  struct thread_info* step_through = pool->thread_list;
  struct thread_info* temp;

  while(step_through != NULL){

    if(pthread_join((step_through->thread), NULL) != 0){
      printf("ERROR: %s\n", strerror(errno));
    }     
    
    temp = step_through;
    step_through = step_through->next;
    free(temp);
  }    
  
  free(pool);
  pool = NULL;
  return;
}

/*Closes threads immediately.
*/
void destroy_pool_immediately(struct thread_pool* pool){

  if(pool == NULL){
    printf("ERROR: Parameter is not a valid thread_pool\n");
    return;
  }
   
  //stop the threads from idling or finish when done with current task
  close_immediately(pool);

  //free the linked list pointed to by pool->head_of_thread_info
  struct thread_info* step_through = pool->thread_list;
  struct thread_info* temp;

  while(step_through != NULL){

    if(pthread_join((step_through->thread), NULL) != 0){
      printf("ERROR: %s\n", strerror(errno));
    }     
    
    temp = step_through;
    step_through = step_through->next;
    free(temp);
  }    

  free(pool);
  pool = NULL;
  return;
}

void set_comparision(int (*function)(const void* p1, const void* p2), struct thread_pool* pool){

  pool->comp_function = function;
  return;
}

//------------------------------------------------------

