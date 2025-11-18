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

#define mob_array_header(array) ((MobArrayHeader *)((void*)(array) - sizeof(MobArrayHeader)))
#define mob_array_len(array)    ((array) ? mob_array_header(array)->len : 0)
#define mob_array_cap(array)    ((array) ? mob_array_header(array)->cap : 0)

#define array_grow(arena, array, n) ((array) = mob_array_grow((arena), (array), sizeof *(array), (n)))
#define array_push(arena, array, element)   \
    (array_grow(arena, array, 1),       \
     (array)[mob_array_header(array)->len++] = (element))

void *mob_array_grow(Arena *arena, void *array, u64 arr_elem_size, u64 n) {
    if (mob_array_cap(array) >= mob_array_len(array) + n) {
        return array;
    }

    u64 cap = mob_array_cap(array) ? mob_array_cap(array) : 4096;
    while (mob_array_len(array) + n >= cap) {
        cap = cap * 2;
    }

    u8 *b = alloc(arena, u8, arr_elem_size * cap + sizeof(MobArrayHeader), true);
    b += sizeof(MobArrayHeader);

    if (array != NULL) {
        b = memcpy(b, (u8*)array, mob_array_len(array) * arr_elem_size);
    }

    mob_array_header(b)->len = mob_array_len(array);
    mob_array_header(b)->cap = cap;
    return b;
}

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
//#define __HM_HEADER__ struct { \
//
//}

