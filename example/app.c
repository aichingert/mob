#include <stdio.h>
#include <stdlib.h>

#define SET_FN_PTR(name, type) struct \
    { type (*name)(type, type); \
    }

struct A {
    struct {
        int c;
        int d;
    };

    union {
        int a;
        char b;
    };

    SET_FN_PTR(is_eq, Data);
    int (*f)(int, int);
};

struct Data {
    int value;
    int place;
};

int (*f)(int, int) = add;

int main(void) {
    printf("%d\n", f(10, 5));

    return 0;
}
