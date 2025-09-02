#include <stdio.h>

struct Data {
    int value;
    int place;
};

int (*f)(int, int) = ads;

int main(void) {
    printf("%d\n", f(10, 5));

    return 0;
}
