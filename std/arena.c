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

void arena_init(Arena *arena, u64 chunk_size) {
    arena->chunk_size = chunk_size;
    arena->head = NULL;
    arena->curr = NULL;

    assert(chunk_size > 2 << 12, "chunk size should be bigger");

    chunk_size = (sizeof(ArenaChunk) + chunk_size + 4096 - 1) & ~(4096 - 1);
    u8 *mem = os_alloc(chunk_size);

    arena->head = (ArenaChunk*)mem;
    arena->curr = arena->head;
    arena->head->len = 0;
    arena->head->cap = chunk_size;
    arena->head->mem = mem + sizeof(ArenaChunk);
    arena->head->next = NULL;
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
