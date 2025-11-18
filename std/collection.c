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

struct MobArrayHeader {
    u64 len;
    u64 cap;
};

#define mob_array_header(array) ((MobArrayHeader *)((void*)(array) - sizeof(MobArrayHeader) * 8))

#define mob_array_len(array)    ((array) ? mob_array_header(array)->len : 0)

#define mob_array_cap(array)    ((array) ? mob_array_header(array)->cap : 0)

#define array_grow(arena, array, n) 10
#define array_push(arena, array, element) 10

void *mob_array_grow(Arena *arena, void *array, u64 arr_elem_size, u64 n) {
    u64 cap = mob_array_cap(array) ? mob_array_cap(array) : 4096;
    while (cap < n) {
        cap *= 2;
    }

    u8 *b = alloc(arena, u8, arr_elem * cap + sizeof(MobArrayHeader), size, 1, 0);
    b += sizeof(MobArrayHeader) * 8;

    if (array == NULL) {
        b = memcpy(b, (u8*)array, mob_array_len(array) * arr_elem_size);
    }

    return b;
}

void *mob_array_push(Arena *arena, void *array, u64 arr_elem_size, void *element) {

}

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
// struct Type {
//     u8 *key;
//
//     u32 file;
//     u32 token;
// };
//
// struct Types {
//     __HM_HEADER__;
// };
// Types t = {0};
// String s = S("Token");
// TODO: think about functions
// hm_putp(ARENA, &t, s.val, s.len, Types, Types{ .file = 10, .token = 5})
// Types tok = hm_getp(ARENA, &t, s.val, s.len, Types);
#define __HM_HEADER__ struct { \

}

