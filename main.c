#include <stdio.h>
#include <unistd.h>
#include "ReferenceCounter.h"

void doesSomething(int time) {
    int *a = allocate(sizeof(int));
    *a = 5;
    printf("a: %d\n", *a);
    //sleep(time);
}

void longLoop() {
    doesSomething(1);
    updateStackPointer();
    for (int i = 0; i < 100000; i++) {
        //sleep(1);
    }
}

void anotherLongLoop() {
    for (int i = 0; i < 100000; i++) {
        //sleep(1);
    }
}

int * getIntPointer() {
    return allocate(sizeof(int));
}

int ** getDoublePointer() {
    return allocate(sizeof(int*));
}

int main() {
    printf("Hello, World!\n");


    int *a = getIntPointer();
    *a = 5;
    printf("&a: %p\n", &a);
    sleep(3);
    printf("a: %p\n", a);

    printf("a: %d\n", *a);

    int **b = getDoublePointer();
    *b = a;

    *b = getIntPointer();
    **b = 10;

    printf("b: %p\n", b);
    printf("*b: %p\n", *b);
    printf("**b: %d\n", **b);

    deallocate(b);

    //sleep(1);
    /*for (int i = 0; i < 5; i++) {
        doesSomething(i);
        updateStackPointer();
        //sleep(4);
    }
    longLoop();
    //sleep(1);
    updateStackPointer();
    //sleep(10);
    long *val = allocate(sizeof (long));
    updateStackPointer();

    *val = 5;

    printf("%ld\n", *val);

    long *val2 = continuousAllocate(5, sizeof(long));
    updateStackPointer();

    val2[0] = 1;

    longLoop();
    updateStackPointer();

    val2 = reallocate(val2, sizeof(long) * 10);
    updateStackPointer();

    val2[5] = 5;*/

    //anotherLongLoop();
    return 0;
}
