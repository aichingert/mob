#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>

#if !MOB_SELF_BUILD
    #include "arena.h"
#endif

void *arena_alloc(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, bool zero) {
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    ptrdiff_t available = a->end - a->beg - padding;

    if (available < 0 || count > available / size) {
        assert(false && "TODO: realloc");
    }

    void *p = a->beg + padding;
    a->beg += padding + count * size;

    if (!zero) {
        return p;
    }

    return memset(p, 0, count * size);
}

void grow(void *slice, ptrdiff_t size, Arena *a) {
    struct {
        void     *data;
        ptrdiff_t len;
        ptrdiff_t cap;
    } replica;
    memcpy(&replica, slice, sizeof(replica));

    replica.cap = replica.cap ? replica.cap : 1;
    ptrdiff_t align = 16;
    void *data = arena_alloc(a, 2*size, align, replica.cap, true);
    replica.cap *= 2;
    if (replica.len) {
        memcpy(data, replica.data, size*replica.len);
    }
    replica.data = data;
    memcpy(slice, &replica, sizeof(replica));
}

