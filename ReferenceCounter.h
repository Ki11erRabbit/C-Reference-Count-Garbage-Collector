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


typedef struct Pointer {
    void *ptr;
    int refCount;
    int foundRefCount;
    struct Pointer *next;
    struct Pointer *prev;
} Pointer;

static void *stackPointer = NULL;
static int ended = 0;
static Pointer *head = NULL;
static Pointer *tail = NULL;


int rc_main(int argc, char** argv);

#define main(...) \
  main(int argc, char** argv) { \
    long stk = (long)NULL;            \
    pthread_t gcThread;         \
    pthread_create(&gcThread,NULL,garbageCollector,(void *)&stk);\
    int return_val = rc_main(argc, argv);                        \
    sleep(0);\
    signal_end();              \
    pthread_join(gcThread,NULL);              \
    return return_val;\
  }; \
  int rc_main(int argc, char** argv)


void *allocate(size_t size);
void *garbageCollector(void *stackEnd);
void mark_stack();
void signal_end();
void incrementFound(void* pointerToTest);
void updateStackPointer();




#endif //REFCOUNTGC_REFERENCECOUNTER_H
