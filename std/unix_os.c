#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_NONE   0x0

#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02
#define MAP_SHARED_VALIDATE 0x03
#define MAP_DROPPABLE 0x08
#define MAP_TYPE 0x0f
#define MAP_FIXED 0x10
#define MAP_FILE 0
#define MAP_ANON    0x20

u8 *os_alloc(u64 bytes) {
    return sys_mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

s32 os_free(u8 *mem, u64 bytes) {
    return sys_munmap(mem, bytes);
}
