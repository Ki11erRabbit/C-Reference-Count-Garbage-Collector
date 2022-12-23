//
// Created by ki11errabbit on 12/22/22.
//

#include <unistd.h>
#include "ReferenceCounter.h"


void signal_end() {
    sleep(2);
    ended = 1;
}

void mark_stack(void) {
    long stackTop = (long) NULL;

    stackPointer = &stackTop;
}

void updateStackPointer() {
    volatile int noinline = 1;
    void (*stack_mark)(void) = noinline
                               ? mark_stack
                               : (void(*)(void))(NULL);

    stack_mark();
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void addPointerToList(void *ptr) {
    Pointer *newPointer = (Pointer *) malloc(sizeof(Pointer));
    newPointer->ptr = ptr;
    newPointer->refCount = 1;
    newPointer->foundRefCount = 0;
    if (tail == NULL) {
        newPointer->next = NULL;
        newPointer->prev = NULL;
        head = newPointer;
        tail = newPointer;
    }
    else {
        newPointer->next = NULL;
        newPointer->prev = tail;
        tail->next = newPointer;
        tail = newPointer;
    }
}

void* allocate(size_t size) {
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);

    updateStackPointer();

    void *ptr = malloc(size);

    addPointerToList(ptr);

    return ptr;
}

void incrementFound(void* pointerToTest) {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if ((size_t)currPointer->ptr == (size_t)pointerToTest) {
            printf("Marking %p\n", pointerToTest);
            currPointer->foundRefCount++;
            return;
        }
        currPointer = currPointer->next;
    }
}

void findPointers(void *stackStart) {
    if (head == NULL) {
        return;
    }
    Pointer *currPointer = head;
    if (stackPointer < stackStart) {
        for (void *p = stackStart; (size_t)p >= (size_t)stackPointer; p = ((char*)p) - sizeof(size_t)) {
            incrementFound(p);
            if ((size_t)p == (size_t)stackPointer) {
                break;
            }
        }
    }
    if (stackPointer > stackStart) {
        for (void *p = stackStart; (size_t)p <= (size_t)stackPointer; p = ((char*)p) +  sizeof(size_t)) {
            incrementFound(p);
            if ((size_t)p == (size_t)stackPointer) {
                break;
            }
        }
    }
}

void freePointers() {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if (currPointer->refCount != currPointer->foundRefCount) {
            if (currPointer->foundRefCount == 0) {
                fprintf(stderr,"Freeing %p\n", currPointer->ptr);
                free(currPointer->ptr);
                Pointer *nextPointer = currPointer->next;
                Pointer *prevPointer = currPointer->prev;
                if (nextPointer == NULL) {
                    tail = prevPointer;
                } else {
                    nextPointer->prev = prevPointer;
                }
                if (prevPointer == NULL) {
                    head = nextPointer;
                } else {
                    prevPointer->next = nextPointer;
                }
                free(currPointer);
                currPointer = nextPointer;
            }
            else {
                currPointer->refCount = currPointer->foundRefCount;
                currPointer->foundRefCount = 0;
                currPointer = currPointer->next;
            }

        }
        else {
            currPointer->foundRefCount = 0;
            currPointer = currPointer->next;
        }
    }
}

void freeRemaining() {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        printf("Freeing %p\n", currPointer->ptr);
        free(currPointer->ptr);
        Pointer *nextPointer = currPointer->next;
        free(currPointer);
        currPointer = nextPointer;
    }
    head = NULL;
    tail = NULL;
}

void *garbageCollector(void* ptr) {
    void *stackStart = NULL;
    stackStart = ptr;


    printf("Stack start:   %p\n", stackStart);
    void *tempPointer = stackPointer;
    printf("Stack pointer: %p\n", stackPointer);
    while (!ended) {
        if (stackPointer != tempPointer) {
            printf("Stack pointer: %p\n", stackPointer);
            tempPointer = stackPointer;
        }
        //printf("Stack pointer:   %p\n", stackPointer);
        //addPointers(head, tail);

        findPointers(stackStart);

        freePointers();
    }


    //freeRemaining(head, tail);


    return NULL;
}