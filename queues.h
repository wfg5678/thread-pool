/*

This header contains the functions for the different types of queues 
that are supported with the thread pool. The five options are:

1. Binary Heap
2. Binomial Heap
3. Fibonacci Heap
4. First In First Out Queue
5. Last In First Out Queue

 */

#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

//--------Binary Heap Function Declarations
void binary_swap(struct task* a, struct task* b);
struct task* binary_find_task(struct thread_pool* pool, int position);
struct task* binary_h_p_child(struct task* parent, struct thread_pool* pool);
void binary_bubble_up(struct task* new_task, struct thread_pool* pool);
void binary_bubble_down(struct thread_pool* pool);
void binary_push_task(struct task* to_add, struct thread_pool* pool);
struct task* binary_pull_task(struct thread_pool* pool);

//--------Binomial Heap Function Declarations
void binomial_make_child(struct task** ref_child, struct task* parent);
void binomial_combine(struct task** ref_prev, struct task* curr, struct thread_pool* pool);
struct task* binomial_simple_merge(struct task* h1, struct task* h2);
struct task* binomial_consolidate(struct task* head, struct thread_pool* pool);
struct task* binomial_make_union(struct task* h1, struct task* h2, struct thread_pool* pool);
void binomial_push_task(struct task* to_add, struct thread_pool* pool);
struct task* binomial_pull_task(struct thread_pool* pool);

//--------Fibonacci Heap Function Declarations
struct task* fibonacci_make_child(struct task* a, struct task* b, struct thread_pool* pool);
struct task* fibonacci_relink(struct task* ptrs[], int length, struct thread_pool* pool);
void fibonacci_consolidate(struct thread_pool* pool);
void fibonacci_splice(struct task* a, struct task* b);
void fibonacci_set_parent_null(struct task* a);
void fibonacci_unsplice(struct task* a);
void fibonacci_push_task(struct task* to_add, struct thread_pool* pool);
struct task* fibonacci_pull_task(struct thread_pool* pool);

//--------FIFO Function Declarations
void FIFO_push_task(struct task* to_add, struct thread_pool* pool);
struct task* FIFO_pull_task(struct thread_pool* pool);

//--------LIFO Function Declarations
void LIFO_push_task(struct task* to_add, struct thread_pool* pool);
struct task* LIFO_pull_task(struct thread_pool* pool);



//==================Binary Heap Functions==========================
/*
  For Binary Heap functions 'pointer1' refers to the task's left child
  'pointer2' refers to the right child.
*/


//swaps the arguments and functions associated with two tasks
void binary_swap(struct task* a, struct task* b){

  void* temp_arg;
  void (*temp_function)(void*);

  temp_arg = a->arg;
  a->arg = b->arg;
  b->arg = temp_arg;
  
  temp_function = a->function;
  a->function = b->function;
  b->function = temp_function;

  return;
}

/*Works from pool->head down through tree until locating the task in
'position'. If 'position' == 7 the function will return a pointer to 
fourth element in the third row.
The function works by utilizing the binary representation of 'position'.
Consider 'position' == 11. The binary representation is 0000...00 1011b.
To find the path from the head to the 11th task work right from most 
significant digit until encountering a 1. Move to next digit:

If digit == 0 move to left child
If digit == 1 move to right child
Move to next digit and repeat until digits are exhausted

For 'position' == 11 the path is

 head->left child->right child->right child
 */
struct task* binary_find_task(struct thread_pool* pool, int position){

  int path = position;
  unsigned int mask = 0x80000000;
  struct task* curr = pool->head;

  //find the left most 1
  while((path&mask) == 0){

    mask = mask>>1;
  }

  //ignore left most 1
  mask = mask>>1;

  //move left with 0. move right with 1
  while(mask>0){
    if((path&mask) == 0){
      curr = curr->pointer1;
    }
    else{
      curr= curr->pointer2;
    }
    mask = mask>>1;
  }

  return curr;
}

/*Examines children of a task and returns a pointer to the child of
higher priority. Returns NULL if no children exist.
*/
struct task* binary_h_p_child(struct task* parent, struct thread_pool* pool){

  if(parent->pointer1 != NULL){

    if(parent->pointer2 == NULL){
      return parent->pointer1;
    }
    else{
      if(pool->comp_function(parent->pointer1->arg, parent->pointer2->arg) >= 0){
	return parent->pointer1;
      }
      else{
	return parent->pointer2;
      }
    }
  }
  else{
    return NULL;
  }
}

/*Restores max heap property by comparing child to parent. Swap and move up
until max heap restored
*/  
void binary_bubble_up(struct task* to_add, struct thread_pool* pool){

  struct task* curr = to_add;
  
  while(curr->parent != NULL){

    if(pool->comp_function(curr->arg, curr->parent->arg) > 0){
      binary_swap(curr, curr->parent);
      curr = curr->parent;
    }
    else{
      break;
    }
  }
  return;
}

/*Pushes pool->head down until the max heap property is 
restored.
*/
void binary_bubble_down(struct thread_pool* pool){

  struct task* next;
  struct task* curr = pool->head;

  while(1){
    next = binary_h_p_child(curr, pool);
    if(next == NULL){
      break;
    }
    else if(pool->comp_function(next->arg, curr->arg) > 0){
      binary_swap(next, curr);
      curr = next;
    }
    else{
      break;
    }
  }
  return;
}

/*
Pushes task into Binary Heap. If not pool->head is not NULL 'to_add'
is inserted in the first open space. 'to_add' is then exchanged with
it's parent until the heap retains it's max heap property.
*/
void binary_push_task(struct task* to_add, struct thread_pool* pool){

  to_add->pointer1 = NULL;
  to_add->pointer2 = NULL;
  to_add->parent = NULL;
  
  if(pool->head == NULL){

    pool->head = to_add;
    to_add->parent = NULL;
    return;
  }
  else{

    //located the parent task
    int parent_position = pool->num_tasks_in_queue/2;
    struct task* parent = binary_find_task(pool, parent_position);

    //Even numbered tasks are always left children
    if((pool->num_tasks_in_queue)%2 == 0){
      parent->pointer1 = to_add;
    }
    //Odd numbered tasks are always right children
    else{
      parent->pointer2 = to_add;
    }
    
    to_add->parent = parent;

    binary_bubble_up(to_add, pool);
    
  }

  return;
}

/*Function returns the task pointed to by pool->head. This is the highest
priority task. The last task is swapped with the head. The former head is
seperated from heap and the new head is pushed down until the max heap
property is restored.
*/
struct task* binary_pull_task(struct thread_pool* pool){

  struct task* curr = pool->head;

  //If only one task in heap
  if(curr->pointer1 == NULL){
    pool->head = NULL;
    return curr;
  }

  //find the last task
  curr = binary_find_task(pool, pool->num_tasks_in_queue);
  
  //swap last and head
  binary_swap(curr, pool->head);
  
  //seperate curr (now highest priority task)
  //The modular test determines if curr is a right or left child
  if(pool->num_tasks_in_queue%2 == 0){
    curr->parent->pointer1 = NULL;
  }
  else{
    curr->parent->pointer2 = NULL;
  }
  
  curr->parent = NULL;
    
  binary_bubble_down(pool);

  return curr;
}



//====================Binomial Heap Functions======================
/*
  For Binomial Heap functions 'pointer1' refers to the task's sibling.
*/

/*Accepts a reference to a task, 'ref_child' that is to be made into
 a child of the task 'parent'. The child task must be the parent 
task's rightmost child. The if ... else statement ensures this is 
the case.
*/ 
void binomial_make_child(struct task** ref_child, struct task* parent){

  struct task* child = (*ref_child);

  //cut the child task from the root list
  (*ref_child) = child->pointer1;
  child->parent = parent;

  if(parent->child == NULL){
    parent->child = child;
    child->pointer1 = NULL;
  }
  else{
    struct task* prev = parent->child;
    struct task* next = parent->child->pointer1;
    while(next != NULL){
      prev = prev->pointer1;
      next = next->pointer1;
    }
    prev->pointer1 = child;
    child->pointer1 = NULL;
  }
  parent->order++;

  return;
}
  

/*binomial_make_child compares two tasks of equal order and makes the
lower priority task the child of the higher priority task. The two 
tasks compared are 'curr' and 'curr->pointer1', the right sibling of 
'curr'. The pointer to a pointer 'ref_prev' points to a pointer to 
'curr'. This allows 'curr' to be removed from the root list if it is
lower in priority.
 */
void binomial_combine(struct task** ref_prev, struct task* curr, struct thread_pool* pool){

  if(pool->comp_function(curr->arg, curr->pointer1->arg) >= 0){
   
    binomial_make_child(&(curr->pointer1), curr);
  } 
  else{
        binomial_make_child(ref_prev, curr->pointer1);
  }
  return;
}

/*merges two linked lists by order
 2->4->5-> and 1->3->8->
merges to 1->2->3->4->5->8->
*/
struct task* binomial_simple_merge(struct task* h1, struct task* h2){
  
  struct task* new_head;

  //reference to the pointer where the next node will be appended
  struct task** ref_next = &new_head;

  while(1){

    if(h1 == NULL){
      
      (*ref_next) = h2;  
      return new_head;
    }
    else if(h2 == NULL){
      
      (*ref_next) = h1;
      return new_head;
    }

    if(h1->order <= h2->order){
      
      (*ref_next) = h1;
      h1 = h1->pointer1;
    }
    else{
      
      (*ref_next) = h2;
      h2 = h2->pointer1;
    }

    ref_next = &((*ref_next)->pointer1);
  }
}

/*take a linked list with ascending order and  consolidate into a
linked list that obeys the binomial heap property that every the
linked list can only have at most one of each order task.
For example: 1->1->4->5->5->
will consolidate down to 2->4->6->
*/
struct task* binomial_consolidate(struct task* head, struct thread_pool* pool){

  //keep a reference to the previous task to allow removal of curr task
  struct task** ref_prev = &head;

  struct task* curr = head;

  /*there are three cases to consider as moving through linked list
   1. curr task and curr->pointer1 have different order. In this case
      advance curr.
   2. curr and curr->pointer1  and curr->pointer1->pointer1 have the 
      same order. In this case advance curr.
   3. curr and curr->pointer1 have the same order and curr->pointer1->
      pointer1 does not have the same order as the prev to tasks.
      Make the lower priority of curr and curr->pointer1 the child of
      of the higher priority task.
  */
  
  while(curr->pointer1 != NULL){

    //Case 1
    if(curr->order != curr->pointer1->order){

      curr = curr->pointer1;
      ref_prev = &((*ref_prev)->pointer1);
    }
    else{
      //Case 2
      if(curr->pointer1->pointer1 != NULL && curr->pointer1->pointer1->order == curr->order){
	
      curr = curr->pointer1;
      ref_prev = &((*ref_prev)->pointer1);
      }
      //Case 3
      else{
	binomial_combine(ref_prev, curr, pool);
	curr = (*ref_prev);
      }
    }
  }

    return head;
}

struct task* binomial_make_union(struct task* h1, struct task* h2, struct thread_pool* pool){
  struct task* head = binomial_simple_merge(h1, h2);  
  head = binomial_consolidate(head, pool);

  return head;
}

void binomial_push_task(struct task* to_add, struct thread_pool* pool){
 
  to_add->order = 0;
  to_add->pointer1 = NULL;
  to_add->parent = NULL;
  to_add->child = NULL;

  if(pool->head == NULL){
    pool->head = to_add;
  }
  else{
    pool->head = binomial_make_union(pool->head, to_add, pool);
    
  }
  return;
}

struct task* binomial_pull_task(struct thread_pool* pool){

  struct task* highest_priority = pool->head;
  struct task** ref_prev_high_p = &(pool->head);
  struct task* curr = pool->head->pointer1;

  if(curr == NULL){

    pool->head = highest_priority->child;
    return highest_priority;
  }
  struct task** ref_prev = &(pool->head->pointer1);
			      
  while(curr != NULL){

    if(pool->comp_function(curr->arg, highest_priority->arg) > 0){

      ref_prev_high_p = ref_prev;
      highest_priority = curr;
    }

    curr = curr->pointer1;
    ref_prev = &((*ref_prev)->pointer1);
  }

  (*ref_prev_high_p) = (highest_priority->pointer1);
  struct task* new_task = binomial_make_union(pool->head, highest_priority->child, pool);
  pool->head = new_task;
  
  highest_priority->pointer1 = NULL;
  highest_priority->child = NULL;
  highest_priority->parent = NULL;

  return highest_priority;
}


//====================Fibonacci Heap Functions=====================

/*
  For Fibonacci Heap functions 'pointer1' refers to the task's left
  child pointer and 'pointer2' refers to the right child pointer.
*/


/*
  Makes task 'b' into a child of task 'a'. If 'a' has children 'b'
  will be spliced into the child list.
*/
struct task* fibonacci_make_child(struct task* a, struct task* b, struct thread_pool* pool){

  //remove b from root list
  struct task* left_neighbor = b->pointer1;
  struct task* right_neighbor = b->pointer2;

  if(a->child == NULL){
    a->child = b;
    b->pointer1 = b;
    b->pointer2 = b;
  }
  else{
    right_neighbor = a->child;
    left_neighbor = right_neighbor->pointer1;
    left_neighbor->pointer2 = b;
    b->pointer1 = left_neighbor;
    right_neighbor->pointer1 = b;
    b->pointer2 = right_neighbor;
  }
  b->parent = a;
  a->degree++;
  return a;
}

/*
  Relink the tasks pointed to by 'ptrs' into a doubly linked list.
  Return the highest priority task.
*/
struct task* fibonacci_relink(struct task* ptrs[], int length, struct thread_pool* pool){
  
  struct task* high_priority = NULL;
  
  struct task* left_head = NULL;
  struct task** work_right_ref = &left_head;
  for(int i=0; i<length; i++){

    if(ptrs[i] != NULL){
      if(high_priority == NULL || pool->comp_function(ptrs[i]->arg, high_priority->arg) > 0){
	high_priority = ptrs[i];
      }
      (*work_right_ref) = ptrs[i];
      work_right_ref = &((*work_right_ref)->pointer2);
    }
  }
  
  struct task* right_head = NULL;
  struct task** work_left_ref = &right_head;
  for(int i=length-1; i>=0; i--){

    if(ptrs[i] != NULL){
      (*work_left_ref) = ptrs[i];
      work_left_ref = &((*work_left_ref)->pointer1);
    }
  }

  (*work_right_ref) = left_head;
  (*work_left_ref) = right_head;

  return high_priority;
}

/*
  The work of preserving the properties of the fibonacci heap is 
  deferred until a task is removed. At that point nodes of the same
  degree must be consolidated until there is only one node of each
  degree. To accomplish this an array of pointers is created, 'ptrs'.
  ptrs[0] points to a task with degree '0'. ptrs[1] points to a task
  with degree '1' etc. Step through the root list and link each task
  to the appropriate indice of array 'ptrs'. If that indice of 'ptrs'
  already points to a task make the lower priority task into a child
  of the higher priority task.
  When the whole root list has been stepped through relink the tasks
  pointed to by 'ptrs' into a doubly linked list.
  Track the highest priority node and set pool->head to it
*/
void fibonacci_consolidate(struct thread_pool* pool){

  int length = 2;
  while(length < pool->num_tasks_in_queue){
    length = length*2;
  }
    
  struct task* ptrs[length];
  for(int i=0; i<length; i++){
    ptrs[i] = NULL;
  }

  struct task* x = pool->head;
  struct task* y;
  int degree;

  //break the continuous linked list
  x->pointer1->pointer2 = NULL;

  while(x != NULL){

    degree = x->degree;

    while(ptrs[degree] != NULL){
      y = ptrs[degree];
      if(pool->comp_function(x->arg,y->arg) >= 0){
	x = fibonacci_make_child(x,y,pool);
      }
      else{
	y->pointer2 = x->pointer2;
	y = fibonacci_make_child(y,x,pool);
	x = y;
      }
      ptrs[degree] = NULL;
      degree++;
    }
    ptrs[degree] = x;
    x=x->pointer2;
  }

  //relink into doubly linked list
  pool->head = fibonacci_relink(ptrs, length, pool);
  
  return;
}

//splices task 'b' and siblings into root list with 'a'
void fibonacci_splice(struct task* a, struct task* b){

  struct task* left_end = b;
  struct task* right_end = left_end->pointer1;

  //insert into root list
  struct task* right_neighbor = a->pointer2;
  a->pointer2 = left_end;
  left_end->pointer1 =a;
  right_neighbor->pointer1 = right_end;
  right_end->pointer2 = right_neighbor;

  return;
}
  
/*
  Sets the parent pointer of 'a' and all of the siblings of 'a' to
  NULL
*/
void fibonacci_set_parent_null(struct task* a){

  struct task* left_end = a;
  struct task* stop = left_end;
  do{
    left_end->parent = NULL;
    left_end = left_end->pointer2;
  }while(left_end != stop);

  return;
}

//unsplice 'a' from root list
void fibonacci_unsplice(struct task* a){

  struct task* right_neighbor = a->pointer2;
  struct task* left_neighbor = a->pointer1;
  right_neighbor->pointer1 = left_neighbor;
  left_neighbor->pointer2 = right_neighbor;
  return;
}

/*
  Adds a task to the fibonacci heap. Simply splices the task into the
  root list. Updates pool->head if the task just added is the highest
  priority task.
*/
void fibonacci_push_task(struct task* to_add, struct thread_pool* pool){

  to_add->pointer1 = to_add;
  to_add->pointer2 = to_add;
  to_add->parent = NULL;

  if(pool->head == NULL){

    pool->head = to_add;
  }
  else{
    
    fibonacci_splice(pool->head, to_add);

    if(pool->comp_function(to_add->arg, pool->head->arg) > 0){
      pool->head = to_add;
    }
  }

  return;
}

struct task* fibonacci_pull_task(struct thread_pool* pool){

  if(pool->head == NULL){
    return NULL;
  }
  
  struct task* to_return = pool->head;

  //if they exist promote the children of node to be
  //removed to the root list
  if(to_return->child != NULL){

    fibonacci_set_parent_null(to_return->child);
    fibonacci_splice(to_return, to_return->child);
  }

  if(to_return->pointer2 == to_return){
    pool->head = NULL;
  }
  else{

    //update head
    pool->head = to_return->pointer1;
    
    //remove to_return
    fibonacci_unsplice(to_return);

    fibonacci_consolidate(pool);
  }
  return to_return;

}

//====================FIFO Functions===============================

/*
  For First In First Out functions 'pointer1' refers to the task's 
  newer sibling and 'pointer2' refers older sibling.
*/

void FIFO_push_task(struct task* to_add, struct thread_pool* pool){

  if(pool->head == NULL){
    pool->head = to_add;
    to_add->pointer1 = NULL;
    pool->tail = to_add;
    to_add->pointer2 = NULL;
  }
  else{
    to_add->pointer2 = pool->tail;
    to_add->pointer1 = NULL;
    pool->tail->pointer1 = to_add;
    pool->tail = to_add;
  }
  return;
  
}

struct task* FIFO_pull_task(struct thread_pool* pool){
  
  struct task* to_return = pool->head;

  pool->head = pool->head->pointer1;

  //Just dequeued the only element in linked list
  if(pool->head == NULL){
    pool->tail = NULL;
  }
  else{
    pool->head->pointer2 = NULL;
  }

  return to_return;
}

//====================LIFO Functions===============================

/*
  For Last In First Out functions 'pointer1' refers to the next 
  task in line for execution. 
*/


void LIFO_push_task(struct task* to_add, struct thread_pool* pool){

  struct task* temp = pool->head;
  pool->head = to_add;
  to_add->pointer1 = temp;
  return;
}

struct task* LIFO_pull_task(struct thread_pool* pool){

  if(pool->head == NULL){
    printf("ERROR: Attempted to remove task when no task is in queue.");
    return NULL;
  }

  struct task* to_return = pool->head;
  pool->head = to_return->pointer1;

  return to_return;
}
