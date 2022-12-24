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

int main() {
    printf("Hello, World!\n");

    //sleep(1);
    for (int i = 0; i < 5; i++) {
        doesSomething(i);
        //sleep(4);
    }
    longLoop();
    //sleep(1);
    updateStackPointer();
    //sleep(10);
    long *val = allocate(sizeof (long));

    *val = 5;

    printf("%ld\n", *val);

    long *val2 = continuousAllocate(5, sizeof(long));

    val2[0] = 1;

    longLoop();

    val2 = reallocate(val2, sizeof(long) * 10);

    val2[5] = 5;

    //anotherLongLoop();
    return 0;
}
