#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#if __unix__ 
#include <sys/mman.h>
#endif

static const char *path = "app.c";

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;
typedef float       f32;
typedef double      f64;

typedef struct Arena {
    u64 cnt;
    u64 cap;
    u8 *mem;
} Arena;

void allocate(Arena *arena, u64 bytes) {
    arena->cnt = 0;
    arena->cap = bytes;
#if __unix__
    arena->mem = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
}

void *alloc(Arena *arena, u64 bytes, u64 align, u64 count, bool zero) {
    u64 padding = -(u64)arena->mem & ((s64)align - 1);

    if (arena->cap < arena->cnt + padding + bytes * count) {
        while (arena->cap < arena->cnt + padding + bytes * count) {
            arena->cap <<= 1;
        }

        Arena cpy = {0};
        allocate(&cpy, arena->cap << 1);
        memcpy(cpy.mem, arena->mem, arena->cnt);

#if __unix__
        s32 res = munmap(arena->mem, arena->cap);
#endif
        arena->mem = cpy.mem;
    }

    u8* assigned = arena->mem + padding;
    u64 mem_size = bytes * count + padding;
    arena->mem += mem_size; 
    arena->cnt += mem_size;

    if (!zero) {
        return assigned;
    }

    return memset(assigned, 0, count * bytes);
}

s32 main(s32 argc, const char **argv) {
    Arena app = {0};
    allocate(&app, 10);

    return 0;
}

