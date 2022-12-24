//
// Created by ki11errabbit on 12/22/22.
//

#include <unistd.h>
#include "ReferenceCounter.h"


void signal_end() {
    sleep(2);
    ended = 1;
}

static void mark_stack(void) {
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

void setHeapStart() {
    long* heapStartPtr = malloc(sizeof(long));
    heapStart = heapStartPtr;
    free(heapStartPtr);
}


static void addPointerToList(void *ptr) {
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

    if (ptr > heapEnd) {
        heapEnd = ptr;
    }

    return ptr;
}

void *reallocate(void *ptr, size_t size) {
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);

    updateStackPointer();

    void *newPtr = realloc(ptr, size);

    Pointer *current = head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            current->ptr = newPtr;
            break;
        }
        current = current->next;
    }

    if (newPtr > heapEnd) {
        heapEnd = newPtr;
    }

    return newPtr;
}

void *continuousAllocate(size_t num, size_t size) {
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);

    updateStackPointer();

    void *ptr = calloc(num, size);

    addPointerToList(ptr);

    if (ptr > heapEnd) {
        heapEnd = ptr;
    }

    return ptr;
}

static void incrementFound(void* pointerToTest) {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if (currPointer->ptr == pointerToTest) {
            printf("Marking %p\n", pointerToTest);
            currPointer->foundRefCount++;
            return;
        }
        currPointer = currPointer->next;
    }
}

static void findInStack(void *stackStart) {
    long *stack = (long *) stackStart;
    long *stackEnd = (long *) &stackPointer;

    while (stack < stackEnd) {
        printf("checking %p\n", (void*)*stack);
        incrementFound((void *) *stack);
        stack++;
    }
}

static void findPointers(void *stackStart) {
    if (head == NULL) {
        return;
    }
    Pointer *currPointer = head;


    findInStack(stackStart);
    /*if (stackPointer < stackStart) {
        for (void *p = stackStart; p >= stackPointer; p = ((char*)p) - sizeof(size_t)) {
            //printf("%p\n", (size_t*)p);
            void *ptr = (void *)*(size_t *) p;
            if (ptr == NULL || ptr < heapStart || ptr > heapEnd) {
                continue;
            }
            if (ptr > stackStart || ptr < stackPointer) {
                continue;
            }
            printf("Checking 0x%zx\n", *(size_t*)p);
            incrementFound((void*)*(size_t*)p);
        }
    }
    if (stackPointer > stackStart) {
        for (void *p = stackStart; p <= stackPointer; p = ((char*)p) +  sizeof(size_t)) {

            if ((void*)*(size_t*)p == NULL) {
                continue;
            }
            if ((void*)*(size_t*)p < stackStart || (void*)*(size_t*)p > stackPointer) {
                continue;
            }
            printf("Checking 0x%zx\n", *(size_t*)p);
            incrementFound((void*)*(size_t*)p);
        }
    }*/
}

static void freePointers() {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if (currPointer->refCount != currPointer->foundRefCount) {
            if (currPointer->foundRefCount == 0) {
                printf("Freeing %p\n", currPointer->ptr);
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

static void freeRemaining() {
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

    printf("Heap start:    %p\n", heapStart);
    printf("Heap end:      %p\n", heapEnd);
    printf("Stack start:   %p\n", stackStart);
    void *tempPointer = stackPointer;
    void *tempHeapEnd = heapEnd;
    printf("Stack pointer: %p\n", stackPointer);
    while (!ended) {
        if (stackPointer != tempPointer) {
            printf("Stack pointer: %p\n", stackPointer);
            tempPointer = stackPointer;
        }
        if (heapEnd != tempHeapEnd) {
            printf("Heap end:      %p\n", heapEnd);
            tempHeapEnd = heapEnd;
        }
        //printf("Stack pointer:   %p\n", stackPointer);
        //addPointers(head, tail);

        findPointers(stackStart);

        freePointers();
    }

    printf("Freeing remaining pointers\n");
    freeRemaining();


    return NULL;
}