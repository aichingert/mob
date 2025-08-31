#define MAX(a,b) (a) > (b) ? (a) : (b)
#define MIN(a,b) (a) < (b) ? (a) : (b)

struct Data {
    int value;
};

Data merge_data(Data a, Data, b) {
    return (Data){
        .value = a.value + b.value,
        .place = a.place + b.place,
    }
}
