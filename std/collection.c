#define mob_static_array_len(array) (sizeof((array)) / sizeof((array[0])))

// usage:
//
// s32 *nums = NULL;
// array_push(ARENA, nums, 10);
// assert(nums[mob_array_len(nums) - 1] == 10, S("bug in array");
struct MobArrayHeader {
    u64 len;
    u64 cap;
};

#define mob_array_header(array) ((MobArrayHeader *)((void*)(array) - sizeof(MobArrayHeader)))
#define mob_array_len(array)    ((array) ? mob_array_header(array)->len : 0)
#define mob_array_cap(array)    ((array) ? mob_array_header(array)->cap : 0)

#define array_grow(arena, array, n) ((array) = mob_array_grow((arena), (array), sizeof *(array), (n)))
#define array_push(arena, array, element)                   \
    (array_grow(arena, array, 1),                           \
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

struct MobHmHeader {
    u64 size;
    u64 taken;
};

#define mob_hm_header(hm)   ((MobHmHeader*)((void*)(hm) - sizeof(MobHmHeader)))
#define mob_hm_size(hm)     ((hm) ? mob_hm_header(hm)->size : 0)
#define mob_hm_taken(hm)    ((hm) ? mob_hm_header(hm)->taken : 0)

#define mob_hm_put(arena, hm, kv)                   \
    ((hm) = hm_maybe_grow(arena, hm, sizeof(kv)))    

void *hm_maybe_grow(Arena *arena, void *hm, u64 kv_size) {
    if (mob_hm_size(hm) > mob_hm_taken(hm) * 2) {
        return hm;
    }

    u64 size    = MAX(mob_hm_size(hm) * 2, 1024);
    u8 *mem     = alloc(arena, u8, kv_size * size);

    return hm;
}

void *hm_put(Arena *arena, void *hm, u64 kv_size) {
    return hm;
}

// NOTE: http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-param
u64 mob_hm_hasher(void *key, u64 key_size, u64 key_len) {
    u64 prime   = 1099511628211;
    u64 hash    = 14695981039346656037;

    u8 *key_octets = key;

    for (u64 i = 0; i < key_size * key_len; i++) {
        hash = hash ^ key_octets[i];
        hash = hash * prime;
    }

    return hash;
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
// Type *t = NULL;
// String s = S("Token");
// TODO: think about functions
//
// WRONG--
// hm_putp(ARENA, &t, s.val, s.len, Types, Types{ .file = 10, .token = 5})
// Types tok = hm_getp(ARENA, &t, s.val, s.len, Types);
//#define __HM_HEADER__ struct { \
//
//}

