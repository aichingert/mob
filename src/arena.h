#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

typedef struct Arena {
    char *beg;
    char *end;
} Arena;

#define alloc(...)            allocx(__VA_ARGS__,alloc4,alloc3,alloc2)(__VA_ARGS__)
#define allocx(a,b,c,d,e,...) e
#define alloc2(a, t)          (t *)arena_alloc(a, sizeof(t), alignof(t), 1, true)
#define alloc3(a, t, n)       (t *)arena_alloc(a, sizeof(t), alignof(t), n, true)
#define alloc4(a, t, n, b)    (t *)arena_alloc(a, sizeof(t), alignof(t), n, b)

#define sizeof(x)    (ptrdiff_t)sizeof(x)
#define countof(a)   (sizeof(a) / sizeof(*(a)))
#define len(s)  (countof(s) - 1)

#define push(s, arena) \
    ((s)->len >= (s)->cap \
        ? grow(s, sizeof(*(s)->data), arena), \
          (s)->data + (s)->len++ \
        : (s)->data + (s)->len++)

void *arena_alloc(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, bool zero);
void grow(void *slice, ptrdiff_t size, Arena *a);


#endif /* ARENA_H */

