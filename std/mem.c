struct Buffer {
    u8 *mem;
    u64 len;
};

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

bool memeql(u8 *cmp, u64 cmp_len, u8 *buf, u64 buf_len) {
    if (cmp_len != buf_len) {
        return false;
    }

    for (u64 i = 0; i < cmp_len; i++) {
        if (cmp[i] != buf[i]) {
            return false;
        }
    }

    return true;
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
                S("buffer out of bounds in mem_write"));    \
        assert(                                             \
                ((u64)buf + *size) % sizeof(value) == 0,    \
                S("invalid buffer alignment in mem_write"));\
        *(type*)(buf + *size) = value;                      \
        *size += sizeof(value);                             \
    }                                                       \

CREATE_MEM_WRITE_FUNC(u8);
CREATE_MEM_WRITE_FUNC(u16);
CREATE_MEM_WRITE_FUNC(u32);
CREATE_MEM_WRITE_FUNC(u64);

