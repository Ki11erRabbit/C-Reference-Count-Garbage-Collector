//
// Created by ki11errabbit on 12/22/22.
//

#ifndef REFCOUNTGC_REFERENCECOUNTER_H
#define REFCOUNTGC_REFERENCECOUNTER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <setjmp.h>
#include <string.h>

/*
 * A simple doubly linked list to store pointers and their reference counts and found reference counts.
 * A found reference count is the number of times a pointer is found in the stack or heap.
 * The reference count is the number of times a pointer appears in either the stack or the heap and if it reaches
 * 0 the pointer is automatically freed.
 *
 * A map or a hash table might be a better data structure but I wanted to keep it simple.
 */
typedef struct Pointer Pointer;
/*
 * A static variable that stores the top of the stack
 */
static void *stackPointer = NULL;
/*
 * A boolean that is set to 0 for the gc to run. It is set to 1 when signal_end is called.
 */
static int ended = 0;
/*
 * A static variable that stores the head of the linked list.
 */
static Pointer *head = NULL;
/*
 * A static variable that stores the tail of the linked list.
 */
static Pointer *tail = NULL;
/*
 * A static variable that stores the start of the heap.
 * This should only be changed by setHeapStart which only happens once.
 * It should never change after that.
 */
static void * heapStart = NULL;
/*
 * A static variable that stores the end of the heap.
 * This is first set by setHeapStart and then updated by the memory allocating functions if the pointer is bigger than
 * the current heapEnd.
 */
static void * heapEnd = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * This is the function that is run inside of main that gets defined by the user via the macro.
 */
int rc_main(int argc, char** argv);

/*
 * This macro hides the fact that main is being redefined. The macro creates a main function that sets up the
 * garbage collector thread and calls rc_main.
 * The function setHeapStart initializes the heapStart and heapEnd variables.
 * Then the variable stk is used to get a rough estimate of the bottom of the stack.
 * The thread is created and the reference to stk is passed into the thread.
 * then rc_main is called.
 * after rc_main returns, the while loop in the gc is exited and the thread is joined.
 */
#define main(...) \
  main(int argc, char** argv) { \
    setHeapStart();              \
    long stk = (long)NULL;            \
    pthread_t gcThread;         \
    pthread_create(&gcThread,NULL,garbageCollector,(void *)&stk);\
    int return_val = rc_main(argc, argv);                        \
    signal_end();              \
    pthread_join(gcThread,NULL);              \
    return return_val;\
  }; \
  int rc_main(int argc, char** argv)

/*
 * This function is the same as malloc.
 * It also changes the stack pointer to the current top stack which should be a more recent top of the stack.
 */
void *allocate(size_t size);
/*
 * This function is the same as realloc.
 * It also changes the stack pointer to the current top stack which should be a more recent top of the stack.
 */
void *reallocate(void *ptr, size_t size);
/*
 * This function is the same as calloc.
 * It also changes the stack pointer to the current top stack which should be a more recent top of the stack.
 */
void *continuousAllocate(size_t num, size_t size);
/*
 * This function is the same as free.
 * It also changes the stack pointer to the current top stack which should be a more recent top of the stack.
 */
void *deallocate(void *ptr);
/*
 * This function is added for convenience.
 * It adds a non-gc pointer to the list of pointers that are being tracked.
 */
void addReference(void *ptr);
/*
 * This function is added for convenience.
 * It removes a non-gc pointer from the list of pointers that are being tracked.
 */
void removeReference(void *ptr);
/*
 * This is the function that controls the garbage collector loop.
 */
void *garbageCollector(void *stackEnd);
/*
 * This function is used to signal the end to the garbage collector.
 * It changes a static variable that is used as the condition for the garbage collector loop.
 */
void signal_end();
/*
 * This function is used to update the stack pointer.
 * It is called before every allocation and deallocation.
 * It can be called manually if the user wants to update the stack pointer, which is highly recommended.
 */
void updateStackPointer();
/*
 * This function is used to set the heap start and end.
 * It is called in main and should not be called manually.
 * It is only available to be called because it is used in the macro.
 */
void setHeapStart();




#endif //REFCOUNTGC_REFERENCECOUNTER_H
