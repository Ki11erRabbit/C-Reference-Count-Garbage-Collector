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
    //anotherLongLoop();
    return 0;
}
