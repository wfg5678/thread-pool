#ifndef STRUCTS
#define STRUCTS


struct thread_info{

  struct thread_pool* pool;
  pthread_t thread;
  struct thread_info* next;
};

/* For binary heap:
   pointer1 refers to a task's left child
   pointer2 refers to a task's right child

   For binomial heap:
   pointer1 refers to a task's sibling

   For fibonacci heap:
   pointer1 refers to a task's left sibling
   pointer2 refers to a task's right sibling

   For FIFO list:
   'pointer1' refers to 

*/
struct task{

  void (*function)(void* arg);
  void* arg;
  int order;
  int degree;
  struct task* parent;
  struct task* child;
  struct task* pointer1;
  struct task* pointer2;
  struct task* sibling;
};

struct thread_pool{

  pthread_mutex_t modify_pool;
  pthread_cond_t signal_change;
  int number_threads;
  struct task* head;
  struct task* tail;
  unsigned int num_tasks_in_queue;
  struct task* (*pull)(struct thread_pool* pool);
  void (*push)(struct task* to_add, struct thread_pool* pool);
  int (*comp_function)(const void* p1, const void* p2);  
  int kill_immediately;
  int kill_when_idle;
  struct thread_info* thread_list;
};

#endif /*STRUCTS*/
