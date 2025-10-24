// TODO: look up systems page size
#define PAGE_SIZE 4096

struct ArenaChunk {
    u64 len;
    u64 cap;
    u8 *mem;
    ArenaChunk *next;
};

struct Arena {
    u64 chunk_size;
    ArenaChunk *head;
    ArenaChunk *curr;
};

#define allocx(a, b, c, d, e, ...)  e
#define alloc(...)          allocx(__VA_ARGS__, alloc4, alloc3, alloc2)(__VA_ARGS__)
#define alloc2(a, t)        arena_alloc(a, sizeof(t), alignof(t), 1, false)
#define alloc3(a, t, c)     arena_alloc(a, sizeof(t), alignof(t), c, false)
#define alloc4(a, t, c, z)  arena_alloc(a, sizeof(t), alignof(t), c, z)

void arena_init(Arena *arena, u64 chunk_size) {
    arena->chunk_size = chunk_size;
    arena->head = NULL;
    arena->curr = NULL;

    assert(chunk_size > 2 << 12, "chunk size should be bigger");

    u64 aligned = align_to_page_size(chunk_size + sizeof(ArenaChunk));
    u8 *mem = os_alloc(aligned);

    arena->head = (ArenaChunk*)mem;
    arena->curr = arena->head;
    arena->head->len = 0;
    arena->head->cap = aligned - sizeof(ArenaChunk);
    arena->head->mem = mem + sizeof(ArenaChunk);
    arena->head->next = NULL;
}

bool is_power_of_two(u64 value) {
    return value != 0 && (value & (value - 1)) == 0;
}

u64 align_to_page_size(u64 bytes_to_alloc) {
    return (bytes_to_alloc + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

u64 align_forward(u64 ptr, u64 align) {
    assert(is_power_of_two(align), "alignemnt has to be a power of 2");

    u64 mod = ptr & (align - 1);
    if (mod != 0) {
        ptr += align - mod;
    }

    return ptr;
}

void *arena_alloc(Arena *arena, u64 size, u64 align, u64 count, bool zero) {
    ArenaChunk *curr = arena->curr;
    u64 ptr = (u64)curr->mem + curr->len;
    u64 padding = align_forward(ptr, align) - ptr;

    if (curr->cap - curr->len < padding + size * count) {
        padding = 0;

        u64 chunk_size = align_to_page_size(MAX(arena->chunk_size, size * count));
        curr->next = (ArenaChunk*)os_alloc(chunk_size);
        arena->curr = curr->next;
        curr = curr->next;
        curr->len = 0;
        curr->cap = chunk_size - sizeof(ArenaChunk);
        curr->mem = (u8*)arena->curr + sizeof(ArenaChunk);
    }

    u8 *mem = curr->mem + curr->len + padding;
    curr->len += padding + size * count;

    if (zero) {
        memset(mem, 0, size * count);
    }
    return mem;
}

void arena_deinit(Arena *arena) {
    ArenaChunk *ptr = arena->head->next;

    while (ptr != NULL) {
        ArenaChunk *nxt = ptr->next;
        assert(os_free(ptr->mem, ptr->cap) == 0, "free failed");
        ptr = nxt;
    }

    s32 res = os_free((u8*)arena->head, arena->head->cap + sizeof(ArenaChunk));
    assert(res == 0, "head free failed");
}
