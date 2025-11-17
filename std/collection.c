#define __ARRAY_LEN(array) (sizeof((array)) / sizeof((array[0])))

// usage:
//
// struct Numbers {
//     __ARRAY_HEADER__;
//     s32 *arr;
// };
//
// Numbers nums = {0};
// array_push(ARENA, &nums, 10);
// assert(nums.arr[nums.len - 1] == 10, S("bug in array");
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

// usage:
//
// struct Types {
//     __HM_HEADER__;
//     u8 *key;
//
//    // ... fields
//    u32 file;
//    u32 token;
// };
// Types t = {0};
// String s = S("Token");
// TODO: think about functions
// hm_putp(ARENA, &t, s.val, s.len, Types, Types{ .file = 10, .token = 5})
// Types tok = hm_getp(ARENA, &t, s.val, s.len, Types);
#define __HASHMAP_HEADER__ struct { \

}

