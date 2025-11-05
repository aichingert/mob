#define __ARRAY_LEN(array) (sizeof((array)) / sizeof((array[0])))

#define __ARRAY_HEADER__ struct{    \
    u64 len;                        \
    u64 cap;                        \
}

#define __array_grow(arena, array)                              \
    do {                                                        \
        u64 new = (array)->cap;                                 \
        if (new == 0) {                                         \
            new = 4096;                                         \
        } else {                                                \
            new = new * 2;                                      \
        }                                                       \
        (array)->arr = (void*)memcpy(                           \
                alloc((arena), *array, new),                    \
                (u8*)(array)->arr,                              \
                sizeof(*array) * (array)->len);                 \
        (array)->cap = new;                                     \
    } while(0)

#define array_push(arena, array, element)                                   \
    do {                                                                    \
        if ((array)->cap <= (array)->len + 1) {                             \
            __array_grow(arena, array);                                     \
        }                                                                   \
        (array)->arr[(array)->len] = (element);                             \
        (array)->len += 1;                                                  \
    } while (0)


/*
void* array_to_dynamic() {
}

#define array_init(arena, array_type, elements)
    ({array_type __arr = {0}; do {
        array_type arr = {0};

        for (u64 i = 0; i < __ARRAY_LEN((elements)); i++) {
            array_push(arena, &arr, (elements[i]));
        }
    } while(0)})
*/
