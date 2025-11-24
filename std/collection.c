#define mob_static_array_len(array) (sizeof((array)) / sizeof((array[0])))

// usage:
//
// s32 *nums = NULL;
// array_push(ARENA, nums, 10);
// assert(nums[array_len(nums) - 1] == 10, S("bug in array");
struct MobArrayHeader {
    u64 len;
    u64 cap;
};

#define array_header(array) ((MobArrayHeader *)((void*)(array) - sizeof(MobArrayHeader)))
#define array_len(array)    ((array) ? array_header(array)->len : 0)
#define array_cap(array)    ((array) ? array_header(array)->cap : 0)

#define array_grow(arena, array, n) ((array) = mob_array_grow((arena), (array), sizeof *(array), (n)))
#define array_push(arena, array, element)                   \
    (array_grow(arena, array, 1),                           \
     (array)[array_header(array)->len++] = (element))

void *mob_array_grow(Arena *arena, void *array, u64 arr_elem_size, u64 n) {
    if (array_cap(array) >= array_len(array) + n) {
        return array;
    }

    u64 cap = MAX(array_cap(array), 1024);
    while (array_len(array) + n >= cap) {
        cap = cap * 2;
    }

    u8 *b = alloc(arena, u8, arr_elem_size * cap + sizeof(MobArrayHeader), true);
    b += sizeof(MobArrayHeader);

    if (array != NULL) {
        b = memcpy(b, (u8*)array, array_len(array) * arr_elem_size);
    }

    array_header(b)->len = array_len(array);
    array_header(b)->cap = cap;
    return b;
}

struct MobHmHeader {
    // check if slot is available
    u64 *used;
    u64 slots;
    u64 taken;
};

#define hm_header(hm)   ((MobHmHeader*)((void*)(hm) - sizeof(MobHmHeader)))
#define hm_slots(hm)    ((hm) ? hm_header(hm)->slots : 0)
#define hm_taken(hm)    ((hm) ? hm_header(hm)->taken : 0)
#define hm_take_slot(hm, pos) \
                            ((hm) ? mob_hm_take_slot(hm, pos) : 1)
#define hm_is_slot_taken(hm, pos) \
                            ((hm) ? mob_hm_is_slot_taken(hm, pos) : 1)

// TODO: address of could be made with array decaying to ptr
#define hm_grow(arena, hm) \
        ((hm) = mob_hm_maybe_grow(arena, (hm), sizeof((hm)->key), sizeof(*(hm))))
#define hm_put(arena, hm, hm_key, value) \
        (hm_grow(arena, hm), \
        mob_hm_put((hm), &(hm_key), sizeof((hm)->key), &(value), sizeof(*(hm))))
#define hm_get(hm, hm_key) \
        (mob_hm_get((hm), sizeof(*(hm)), &(hm_key), sizeof((hm)->key)))
#define hm_rem(arena, hm, hm_key) \
        (mob_hm_rem((hm), sizeof(*(hm)), &(hm_key), sizeof((hm)->key)))

bool mob_hm_take_slot(void *hm, u64 pos) {
    if (hm_is_slot_taken(hm, pos)) return false;

    u64 arr_idx = pos >> 6;
    u64 bin_idx = pos % 64 == 0 ? 0 : 1UL << (pos % 64);
    hm_header(hm)->used[arr_idx] |= bin_idx;
    return true; 
}

void mob_hm_free_slot(void *hm, u64 pos) {
    u64 all_ones = ~0;
    u64 arr_idx = pos >> 6;
    u64 bin_idx = pos % 64 == 0 ? 0 : 1UL << (pos % 64);
    hm_header(hm)->used[arr_idx] &= (all_ones ^ bin_idx);
}

bool mob_hm_is_slot_taken(void *hm, u64 pos) {
    u64 arr_idx = pos >> 6;
    u64 bin_idx = pos % 64 == 0 ? 0 : 1UL << (pos % 64);
    return (hm_header(hm)->used[arr_idx] & bin_idx) == bin_idx;
}

void *mob_hm_maybe_grow(Arena *arena, void *hm, u64 key_size, u64 kv_size) {
    if (hm_slots(hm) > hm_taken(hm) * 2) {
        return hm;
    }

    u64 size        = MAX(hm_slots(hm) * 2, 1024);
    u64 used_len    = size / 8;
    u64 alloc_len   = sizeof(MobHmHeader) + used_len + kv_size * size;
    u8 *realloc_hm  = alloc(arena, u8, alloc_len, true);
    u64 *used_ptr   = (u64*)realloc_hm;

    realloc_hm      += used_len + sizeof(MobHmHeader);
    hm_header(realloc_hm)->slots    = size;
    hm_header(realloc_hm)->used     = used_ptr;

    u64 offset = 0;
    for (u64 i = 0; i < hm_slots(hm) >> 6; i++) {
        u64 slots = hm_header(hm)->used[i];
        u64 index = offset;

        while (slots) {
            while ((slots & 1) == 0) {
                index += 1;
                slots >>= 1;
            }
            // NOTE: bit scan forward
            //u64 next_bit = __builtin_ctz(slots) + 1;
            //index += next_bit;

            void *key   = hm + index * kv_size;
            void *value = hm + index * kv_size + key_size;

            TestMap *l = key;

            mob_hm_put(realloc_hm, key, key_size, value, kv_size);

            //slots >>= next_bit;
            slots >>= 1;
            index += 1;
        }

        offset += 64;
    }

    return realloc_hm;
}

void mob_hm_put(void *hm, void *key, u64 key_size, void *value, u64 kv_size) {
    u64 hash    = mob_hm_hasher(key, key_size, 1);
    u64 pos     = hash % hm_slots(hm);

    while (hm_is_slot_taken(hm, pos)) {
        pos = (pos + 1) % hm_slots(hm);
    }

    hm_take_slot(hm, pos);
    hm_header(hm)->taken = hm_taken(hm) + 1;
    memcpy(hm + pos * kv_size, key, key_size);
    memcpy(hm + pos * kv_size + key_size, value + key_size, kv_size - key_size);
}

// TODO: allcoate returning struct so it does not 
// changed when implementation inserts it somewhere else
void *mob_hm_rem(void *hm, u64 kv_size, void *key, u64 key_size) {
    if (hm == NULL) {
        return hm;
    }

    u64 hash    = mob_hm_hasher(key, key_size, 1);
    u64 pos     = hash % hm_slots(hm);
    void *ret   = NULL;

    while (hm_is_slot_taken(hm, pos)) {
        mob_hm_free_slot(hm, pos);

        if (memeql(hm + pos * kv_size, key_size, key, key_size)) {
            ret = hm + pos * kv_size;
        } else {
            u8 data[kv_size];
            memcpy(data, hm + pos * kv_size, kv_size);
            mob_hm_put(hm, key, key_size, data, kv_size);
        }

        pos = (pos + 1) % hm_slots(hm);
    }

    return ret;
}

void *mob_hm_get(void *hm, u64 kv_size, void *key, u64 key_size) {
    if (hm == NULL) {
        return hm;
    }

    u64 hash    = mob_hm_hasher(key, key_size, 1);
    u64 pos     = hash % hm_slots(hm);

    while (hm_is_slot_taken(hm, pos)) {
        if (memeql(hm + pos * kv_size, key_size, key, key_size)) {
            return hm + pos * kv_size;
        }
        pos += 1;
    }

    return NULL;
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

