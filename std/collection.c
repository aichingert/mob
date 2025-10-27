struct ArrayHeader {
    __ARRAY_HEADER__;
};

#define __ARRAY_HEADER__ struct{    \
    u64 len;                        \
    u64 cap;                        \
}

#define __array_header_len(array) ((ArrayHeader*)(array))->len
#define __array_header_cap(array) ((ArrayHeader*)(array))->cap
#define __array_grow(arena, array)                              \
    do {                                                        \
        u64 new = __array_header_cap(array);                    \
        if (new == 0) {                                         \
            new = 4096;                                         \
        } else {                                                \
            new = new * 2;                                      \
        }                                                       \
        (array)->val = (void*)memcpy(                           \
                alloc((arena), *array, new),                    \
                (u8*)(array)->val,                              \
                sizeof(*array) * __array_header_len(array));    \
    } while(0)

#define push(arena, array, element)                                         \
    do {                                                                    \
        if (__array_header_cap(array) <= __array_header_len(array) + 1) {   \
            __array_grow(arena, array);                                     \
        }                                                                   \
        (array)->val[__array_header_len(array)] = (element);                \
        (array)->len += 1;                                                  \
    } while (0)
