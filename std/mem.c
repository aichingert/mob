u8* memcpy(u8 *dst, u8 *src, u64 len) {
    for (u64 i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    return dst;
}

u8 *memset(u8 *src, u8 value, u64 len) {
    for (u64 i = 0; i < len; i++) {
        src[i] = value;
    }
    return src;
}

#define CREATE_MEM_WRITE_FUNC(type)                         \
    static inline void mem_write_ ## type(                  \
            u8 *buf,                                        \
            u64 *size,                                      \
            u64 cap,                                        \
            type value                                      \
    ) {                                                     \
        assert(                                             \
                *size + sizeof(value) <= cap,               \
                "buffer out of bounds in mem_write");       \
        assert(                                             \
                ((size_t)buf + *size) % sizeof(value) == 0, \
                "invalid buffer alignment in mem_write");   \
        *(type*)(buf + *size) = value;                      \
        *size += sizeof(value);                             \
    }                                                       \

CREATE_MEM_WRITE_FUNC(u8);
CREATE_MEM_WRITE_FUNC(u16);
CREATE_MEM_WRITE_FUNC(u32);
CREATE_MEM_WRITE_FUNC(u64);

