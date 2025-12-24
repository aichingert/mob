int sub(int a, int b) {
    return a - b;
}

#define MAX(a,b) (a) > (b) ? (a) : (b)
#define MIN(a,b) (a) < (b) ? (a) : (b)

typedef int s32;

int add(int a, int b) {
    return a + b;
}

Data merge_data(Data a, Data b) {
    printf("%d - %d\n", a.value, b.value);

    return (Data){
        .value = a.value + b.value,
        .place = a.place + b.place,
    };
}
