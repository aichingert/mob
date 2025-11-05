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

#define O_RDONLY    0000000
#define O_WRONLY    0000001
#define O_RDWR      0000002

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/*
//#define ASSERT((condition), msg)        \
//    #ifndef RELEASE
//    do {
//        if (!(condition)) {
//            printf("ERROR: %s\n", );
//            sys_exit(1);
//        }
//    } while (0)
*/

void assert(bool condition, String msg) {
    #ifndef DEBUG

    if (!condition) {
        printf("ERROR: %s\n", msg.val);
        sys_exit(1);
    }

    #endif
}

u8 *os_alloc(u64 bytes) {
    return sys_mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

s32 os_free(u8 *mem, u64 bytes) {
    return sys_munmap(mem, bytes);
}

s32 os_read(s32 fd, u8 *mem, u64 bytes) {
    return sys_read(fd, mem, bytes);
}

// TODO: might null terminator bc of sys call
//
// mode are permissions
// r (read):    4
// w (write):   2
// e (execute): 1
//
// owner: r + w + e = x
// group: r + w + e = y
// other: r + w + e = z
// mode = xyz
s32 os_open_file(String file_name, s32 flags, u32 mode) {
    return sys_open((char*)file_name.val, flags, mode);
}

s32 os_close_file(s32 fd) {
    return sys_close(fd);
}

s64 os_file_size(s32 fd) {
    s64 size = sys_lseek(fd, 0, SEEK_END);
    sys_lseek(fd, 0, SEEK_SET);
    return size;
}
