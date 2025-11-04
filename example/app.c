#include <stdio.h>
#include <stdlib.h>

struct Data {
    int value;
    int place;
};

int (*f)(int, int) = add;

int main(void) {
    printf("%d\n", f(10, 5));

    return 0;
}
