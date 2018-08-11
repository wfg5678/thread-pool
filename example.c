/* This example program shows how to use the functions in thread_pool.c 
and thread_pool.h to create a thread_pool that services a queue of tasks.

This example consists of 4 threads in the thread pool. 40 tasks are added
to the pool. Each task consists of using an insert sort routine to sort
an array of between 20000 and 40000 integers. These tasks are stored in a
Fibonacci Heap. The tasks with longer arrays have priority over the tasks
with shorter arrays. 

Here is the output from executing this program. Note that the arrays are
sorted in descending order except for the first four. This is because the
the threads in the thread pool grab tasks as soon as they are added. While 
the four threads are busy with the first four tasks the rest of the tasks 
are added. 

Sorting array of length 26470.
Sorting array of length 24847.
Sorting array of length 25061.
Sorting array of length 25345.
Sorting array of length 39995.
Sorting array of length 38913.
Sorting array of length 38564.
Sorting array of length 38514.
Sorting array of length 38424.
Sorting array of length 37957.
Sorting array of length 37295.
Sorting array of length 37075.
Sorting array of length 33442.
Sorting array of length 33382.
Sorting array of length 33173.
Sorting array of length 33037.
Sorting array of length 32374.
Sorting array of length 31444.
Sorting array of length 30989.
Sorting array of length 30360.
Sorting array of length 30130.
Sorting array of length 29768.
Sorting array of length 29577.
Sorting array of length 29256.
Sorting array of length 28698.
Sorting array of length 28592.
Sorting array of length 26977.
Sorting array of length 25808.
Sorting array of length 25747.
Sorting array of length 24378.
Sorting array of length 24315.
Sorting array of length 22867.
Sorting array of length 22758.
Sorting array of length 22264.
Sorting array of length 21518.
Sorting array of length 21444.
Sorting array of length 21385.
Sorting array of length 21336.
Sorting array of length 20914.
Sorting array of length 20708.

 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_pool.h"

//the mechanism for passing the arguments needed for insert_sort
struct insert_sort_info{

  int* v;
  int left;
  int right;
};


/*
  If first argument is of greater priority return greater or equal
  to 0.
  If second argument is of greater priority return less than 0.
*/
int compare(const void* p1, const void* p2){

  struct insert_sort_info* s1 = (struct insert_sort_info*)p1;
  struct insert_sort_info* s2 = (struct insert_sort_info*)p2;

  int len1 = s1->right - s1->left;
  int len2 = s2->right - s2->left;
  
  return len1-len2;
}




//Just a typical insert sort routine
void insert_sort(void* arg){

  //cast the void pointer back to an insert_sort_info pointer
  //to get the array to be sorted and left and right boundaries
  struct insert_sort_info* info = (struct insert_sort_info*)(arg);

  printf("Sorting array of length %d.\n", info->right-info->left);
  
  int* v = info->v;
  int left = info->left;
  int right = info->right;

  int x;
  int j;
  int i = left+1;
  while(i <= right){
 
	  
    x = v[i];
    j = i-1;

    while(j >= left && v[j] > x){

      v[j+1] = v[j];
      j--;
    }
    v[j+1] = x;
    i++;
  }

  return;
}


int main(){

  srand(time(NULL));
  
  //create thread pool with 4 threads and fibonacci heap for the tasks
  struct thread_pool* pool = create_pool(4,3,compare);

  
  struct insert_sort_info* array[40];
  int r;
  
  for(int i=0; i<40; i++){

    //get a random number between 20000 and 40000
    r = rand()%20000 + 20000;
    
    array[i] = malloc(sizeof(struct insert_sort_info));
    array[i]->v = malloc(sizeof(int)*r);

    //fill the arrays with random numbers
    for(int j=0; j<r; j++){
      array[i]->v[j] = rand();
    }

    //set the left and right boundaries
    array[i]->left = 0;
    array[i]->right = r-1;
   
    //add task to queue
    add_task(pool, insert_sort, (void*)(array[i]));
  }  

  destroy_pool_when_idle(pool);
   
  for(int i=0; i<40; i++){

    free(array[i]->v);
    free(array[i]);
  }
  
 return 0;
}
