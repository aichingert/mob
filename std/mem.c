u8* memcpy(u8 *dst, u8 *src, u64 size) {
    for (u64 i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return dst;
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
        printf("%llu %llu %llu\n", (size_t)buf, *size, sizeof(value)); \
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

