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

    u64 cap = MAX(mob_array_cap(array), 1024);
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
    // check if slot is available
    u64 *used;
    u64 slots;
    u64 taken;
};

#define mob_hm_header(hm)   ((MobHmHeader*)((void*)(hm) - sizeof(MobHmHeader)))
#define mob_hm_slots(hm)    ((hm) ? mob_hm_header(hm)->slots : 0)
#define mob_hm_taken(hm)    ((hm) ? mob_hm_header(hm)->taken : 0)
#define mob_hm_take_slot(hm, pos) \
                            ((hm) ? hm_take_slot(hm, pos) : 1)
#define mob_hm_is_slot_taken(hm, pos) \
                            ((hm) ? hm_is_slot_taken(hm, pos) : 1)

// TODO: address of could be made with array decaying to ptr
#define mob_hm_grow(arena, hm) \
        ((hm) = hm_maybe_grow(arena, (hm), sizeof(*(hm))))
#define mob_hm_put(arena, hm, key, value) \
        (mob_hm_grow(arena, hm), \
        hm_put((hm), &(key), sizeof((hm)->key), &(value), sizeof(*(hm))))
#define mob_hm_get(arena, hm, key) \
        (hm_get((hm), sizeof(*(hm)), &(key), sizeof((hm)->key)))

bool hm_take_slot(void *hm, u64 pos) {
    if (hm_is_slot_taken(hm, pos)) return false;

    u64 arr_idx = pos / 64;
    u64 bin_idx = 1 << (pos % 64);
    mob_hm_header(hm)->used[arr_idx] |= mob_hm_header(hm)->used[arr_idx] | bin_idx;
    return true; 
}

bool hm_is_slot_taken(void *hm, u64 pos) {
    u64 arr_idx = pos / 64;
    u64 bin_idx = 1 << (pos % 64);
    return (mob_hm_header(hm)->used[arr_idx] & bin_idx) == bin_idx;
}

void *hm_maybe_grow(Arena *arena, void *hm, u64 kv_size) {
    if (mob_hm_slots(hm) > mob_hm_taken(hm) * 2) {
        return hm;
    }

    u64 size        = MAX(mob_hm_slots(hm) * 2, 1024);
    u64 used_len    = size / 8;
    u64 alloc_len   = sizeof(MobHmHeader) + used_len + kv_size * size;
    u8 *realloc_hm  = alloc(arena, u8, alloc_len, true);
    u64 *used_ptr   = (u64*)realloc_hm;

    realloc_hm      += used_len + sizeof(MobHmHeader);
    mob_hm_header(realloc_hm)->slots    = size;
    mob_hm_header(realloc_hm)->used     = used_ptr;

    // TODO: copy old data
    // use bit mask to find
    // values
    return realloc_hm;
}

void hm_put(void *hm, void *key, u64 key_size, void *value, u64 kv_size) {
    u64 hash    = mob_hm_hasher(key, key_size, 1);
    u64 pos     = hash % mob_hm_slots(hm);

    while (hm_is_slot_taken(hm, pos)) {
        pos = (pos + 1) % mob_hm_slots(hm);
    }

    printf("%lu %d %d\n", pos, kv_size, kv_size - key_size);
    hm_take_slot(hm, pos);
    mob_hm_header(hm)->taken = mob_hm_taken(hm) + 1;
    TestMap *out = (TestMap*)memcpy(hm + pos * kv_size, key, key_size);
    memcpy(hm + pos * kv_size + key_size, value + key_size, kv_size - key_size);

    printf("%p %d %d\n", out, out->val, out->key);
}

void *hm_get(void *hm, u64 kv_size, void *key, u64 key_size) {
    if (hm == NULL) {
        return hm;
    }

    u64 hash    = mob_hm_hasher(key, key_size, 1);
    u64 pos     = hash % mob_hm_slots(hm);

    while (hm_is_slot_taken(hm, pos)) {
        printf("%lu kv_size:%d\n", pos, kv_size);
        TestMap *out = (TestMap*)(hm + pos * kv_size);
        printf("%p %d %d\n", out, out->key, out->val);
        if (memeql(hm + pos * kv_size, key_size, key, key_size)) {
            return hm + pos * kv_size;
        }
        printf("no match\n");
        pos += 1;
    }

    return hm + pos * kv_size;
}

// NOTE: http://www.isthe.com/chongo/tech/comp/fnv/index.html#FNV-param
u64 mob_hm_hasher(void *key, u64 key_size, u64 key_len) {
    u64 prime   = 1099511628211U;
    u64 hash    = 14695981039346656037U;

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

