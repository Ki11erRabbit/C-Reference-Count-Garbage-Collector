//
// Created by ki11errabbit on 12/22/22.
//

#include "ReferenceCounter.h"

/*
 * A simple doubly linked list to store pointers and their reference counts and found reference counts.
 * A found reference count is the number of times a pointer is found in the stack or heap.
 * The reference count is the number of times a pointer appears in either the stack or the heap and if it reaches
 * 0 the pointer is automatically freed.
 *
 * A map or a hash table might be a better data structure, but I wanted to keep it simple.
 */
struct Pointer {
    void *ptr;
    int refCount;
    int foundRefCount;
    struct Pointer *next;
    struct Pointer *prev;
};

void signal_end() {
    ended = 1;
}

/*
 * This function is used to get the bottom of the stack.
 */
static void mark_stack(void) {
    long stackTop = (long) NULL;

    stackPointer = &stackTop;
}
/*
 * updates the stack pointer.
 * this function is borrowed from libcello.
 * setting noinline as volatile and using a function pointer to call prevents the compiler from optimizing it out.
 */
void updateStackPointer() {
    volatile int noinline = 1;
    void (*stack_mark)(void) = noinline
                               ? mark_stack
                               : (void(*)(void))(NULL);

    stack_mark();
}

/*
 * This function is used to set the start and end of the heap.
 * It is called in main.
 */
void setHeapStart() {
    long* heapStartPtr = malloc(sizeof(long));
    long *heapEndPtr = malloc(sizeof(long));
    heapStart = heapStartPtr;
    heapEndPtr = heapEndPtr;
    free(heapStartPtr);
    free(heapEndPtr);
}

/*
 * This function is used to add a pointer to the end of the linked list.
 */
static void addPointerToList(void *ptr) {
    pthread_mutex_lock(&lock);
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
    pthread_mutex_unlock(&lock);
}

/*
 * This function is the same as malloc.
 * The setting of the jmp_buf is borrowed from libcello.
 * Setting the jump buffer should prevent any pointers from being left in registers.
 */
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

/*
 * This function is the same as realloc.
 * The setting of the jmp_buf is borrowed from libcello.
 * Setting the jump buffer should prevent any pointers from being left in registers.
 * If the pointer is changed then the old pointer is changed to the new pointer.
 */
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

/*
 * This function is the same as calloc.
 * The setting of the jmp_buf is borrowed from libcello.
 * Setting the jump buffer should prevent any pointers from being left in registers.
 */
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

/*
 * This function is the same as free.
 * The setting of the jmp_buf is borrowed from libcello.
 * Setting the jump buffer should prevent any pointers from being left in registers.
 */
void *deallocate(void *ptr) {
#ifdef DEBUG
    printf("Deallocating %p\n", ptr);
#endif
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);

    updateStackPointer();

    Pointer *current = head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            if (current->prev == NULL) {
                head = current->next;
            }
            else {
                current->prev->next = current->next;
            }
            if (current->next == NULL) {
                tail = current->prev;
            }
            else {
                current->next->prev = current->prev;
            }
            free(current);
            break;
        }
        current = current->next;
    }

    free(ptr);
    ptr = NULL;
    return NULL;
}

/*
 * This function adds a pointer to be monitored by the garbage collector.
 * If the pointer is already in the list it is not added, otherwise it is added.
 */
void addReference(void *ptr) {
    Pointer *current = head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            fprintf(stderr,"Already in list\n");
            return;
        }
        current = current->next;
    }
    addPointerToList(ptr);
}

/*
 * This function is used to remove a pointer from the list.
 * It is used to remove pointers that are being monitored by the garbage collector.
 * This has been added in case the gc is freeing a function earlier than expected.
 */
void removeReference(void *ptr) {
    Pointer *current = head;
    while (current != NULL) {
        if (current->ptr == ptr) {
            if (current->prev == NULL) {
                head = current->next;
            }
            else {
                current->prev->next = current->next;
            }
            if (current->next == NULL) {
                tail = current->prev;
            }
            else {
                current->next->prev = current->prev;
            }
            //printf("Pointer %p no longer gc\n", ptr);
            free(current);
            return;
        }
        current = current->next;
    }
    fprintf(stderr,"Pointer %p not in list\n", ptr);
}

/*
 * This function is used to mark the found reference count of the pointers if a given pointer is matched.
 */
static void incrementFound(void* pointerToTest) {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if (currPointer->ptr == pointerToTest) {
            currPointer->foundRefCount++;
#ifdef DEBUG
            printf("Marking %p RefCount = %d\n", pointerToTest, currPointer->foundRefCount);
#endif
            return;
        }
        currPointer = currPointer->next;
    }
}

/*
 * This function is used to see if a pointer is within the heap.
 */
static void findInHeap() {
#ifdef DEBUG
    printf("Finding in heap\n");
#endif
    long *currPtr = (long *) heapStart;

    while (currPtr <= (long *) heapEnd) {
        if (*currPtr > (long) heapStart && *currPtr < (long) heapEnd) {
#ifdef DEBUG
            printf("Checking %p\n", (void *) *currPtr);
#endif
            incrementFound((void *) *currPtr);
        }
        currPtr++;
    }
}

/*
 * This function is used to see if a pointer is within the stack.
 * It should be able to find it whether the stack grows up or down.
 */
static void findInStack(void *stackStart) {
#ifdef DEBUG
    printf("Trying to find in stack\n");
#endif

    if (stackPointer < stackStart) {//for when the heap grows downwards
        long *stack = (long *) stackPointer;
        long *stackEnd = (long *) stackStart;

        while (stack <= stackEnd) {
            void *ptr = (void *) *stack;
            if (ptr == NULL) {
                stack++;
                continue;
            }
            if (ptr >= heapStart && ptr <= heapEnd) {
#ifdef DEBUG
                printf("checking %p\n", (void*)*stack);
#endif
                incrementFound((void *) *stack);
            }

            stack++;
        }
    }

    if (stackPointer > stackStart) {//for when the heap grows upwards
        long *stack = (long *) stackStart;
        long *stackEnd = (long *) stackPointer;

        while (stack <= stackEnd) {
            void *ptr = (void *) *stack;
            if (ptr == NULL) {
                stack++;
                continue;
            }
            if (ptr >= heapStart && ptr <= heapEnd) {
#ifdef DEBUG
                printf("checking %p\n", (void*)*stack);
#endif
                incrementFound((void *) *stack);
            }

            stack++;
        }
    }

}

/*
 * This function is used find function pointers in the stack and the heap.
 * It returns early if the linked list is empty.
 */
static void findPointers(void *stackStart) {
    if (head == NULL) {
        return;
    }
    findInStack(stackStart);
    findInHeap();

}

/*
 * This function is used to free pointers that have a found reference count of 0.
 * It also updates the reference count to match the found reference count.
 * It also resets the found reference count to 0.
 */
static void freePointers() {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
        if (currPointer->refCount != currPointer->foundRefCount) {
            if (currPointer->foundRefCount == 0) {
#ifdef DEBUG
                printf("Freeing %p\n", currPointer->ptr);
#endif
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

/*
 * This function is used to free the pointers that didn't get freed during the loop.
 * This ensures that no memory is leaked at the end of the program.
 */
static void freeRemaining() {
    Pointer *currPointer = head;
    while (currPointer != NULL) {
#ifdef DEBUG
        printf("Freeing %p\n", currPointer->ptr);
#endif
        free(currPointer->ptr);
        Pointer *nextPointer = currPointer->next;
        free(currPointer);
        currPointer = nextPointer;
    }
    head = NULL;
    tail = NULL;
}

/*
 * The main controlling loop of the garbage collector.
 * It takes a pointer to the stack bottom (stackStart).
 * It loops until set_ended is called by main.
 * It calls findPointers and freePointers every loop.
 * Once the loop exits, it frees the remaining pointers if there are any.
 */
void *garbageCollector(void* ptr) {
    void *stackStart = NULL;
    stackStart = ptr;

#ifdef DEBUG
    printf("Heap start:    %p\n", heapStart);
    printf("Heap end:      %p\n", heapEnd);
    printf("Stack start:   %p\n", stackStart);
    printf("Stack pointer: %p\n", stackPointer);
#endif
    void *tempPointer = stackPointer;
    void *tempHeapEnd = heapEnd;
    while (!ended) {
#ifdef DEBUG
        if (stackPointer != tempPointer) {
            printf("Stack pointer: %p\n", stackPointer);
            tempPointer = stackPointer;
        }
        if (heapEnd != tempHeapEnd) {
            printf("Heap end:      %p\n", heapEnd);
            tempHeapEnd = heapEnd;
        }
#endif

        pthread_mutex_lock(&lock);
        findPointers(stackStart);

        freePointers();
        pthread_mutex_unlock(&lock);
    }
#ifdef DEBUG
    printf("Freeing remaining pointers\n");
#endif
    freeRemaining();


    return NULL;
}