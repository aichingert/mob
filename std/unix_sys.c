#define SYS_CALL_READ       0
#define SYS_CALL_OPEN       2
#define SYS_CALL_CLOSE      3
#define SYS_CALL_LSEEK      8
#define SYS_CALL_MMAP       9
#define SYS_CALL_MUNMAP     11
#define SYS_CALL_SOCKET     41
#define SYS_CALL_CONNECT    42
#define SYS_CALL_SENDTO     44
#define SYS_CALL_EXIT       60

void assert(bool condition, const char *msg) {
    #ifndef DEBUG

    if (!condition) {
        printf("ERROR: %s\n", msg);
        sys_exit(1);
    }

    #endif
}

s64 sys_read(s32 fd, void *buf, u64 count) {
    s64 result = 0;

    __asm__ volatile (
            "   movq %[asm_fd],     %%rdi\n"
            "   movq %[asm_buf],    %%rsi\n"
            "   movq %[asm_count],  %%rdx\n"
            "   movl %[sys_call_n], %%eax\n"
            "   syscall\n"
            : "=r" (result)
            :   [asm_fd]    "r" ((u64)fd),
                [asm_buf]   "r" (buf),
                [asm_count] "r" (count),
                [sys_call_n]"r" (SYS_CALL_READ)
            : "%rdi", "%esi", "%edx"
    );

    return result;
}

s32 sys_open(char *path, s32 flags, u32 mode) {
    s32 result = 0;

    __asm__ volatile (
            "   movq %[asm_filename],   %%rdi\n"
            "   movl %[asm_flags],      %%esi\n"
            "   movl %[asm_mode],       %%edx\n"
            "   movl %[sys_call_n],     %%eax\n"
            "   syscall\n"
            : "=r" (result)
            :   [asm_filename]  "r" (path),
                [asm_flags]     "r" (flags),
                [asm_mode]      "r" (mode),
                [sys_call_n]    "r" (SYS_CALL_OPEN)
            : "%rdi", "%esi", "%edx"
    );

    return result;
}

s32 sys_close(s32 fd) {
    s32 result = 0;

    __asm__ volatile (
            "   movl %[asm_fd],     %%edi\n"
            "   movl %[sys_call_n], %%eax\n"
            "   syscall\n"
            : "=r" (result)
            :   [asm_fd]  "r" (fd),
                [sys_call_n]    "r" (SYS_CALL_CLOSE)
            : "%rdi", "%esi", "%edx"
    );

    return result;
}

s64 sys_lseek(s32 fd, s64 offset, s32 whence) {
    s64 result = 0;

    __asm__ volatile (
            "   movq %[asm_fd],     %%rdi\n"
            "   movq %[asm_offset], %%rsi\n"
            "   movq %[asm_whence], %%rdx\n"
            "   movl %[sys_call_n], %%eax\n"
            "   syscall\n"
            : "=r" (result)
            :   [asm_fd]        "r" ((u64)fd),
                [asm_offset]    "r" (offset),
                [asm_whence]    "r" ((u64)whence),
                [sys_call_n] "r"    (SYS_CALL_LSEEK)
            : "%rdi", "%rsi", "%rdx"
    );

    return result;
}

void *sys_mmap(void *addr, u64 length, s32 prot, s32 flags, s32 fd, s64 offset) {
    void *result = NULL;

    __asm__ volatile (
            "   movq %[sys_call_n], %%rax\n"
            "   movq %[asm_addr],   %%rdi\n"
            "   movq %[asm_length], %%rsi\n"
            "   movq %[asm_prot],   %%rdx\n"
            "   movq %[asm_flags],  %%r10\n"
            "   movq %[asm_fd],     %%r8\n"
            "   movq %[asm_offset], %%r9\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n] "r"    ((u64)SYS_CALL_MMAP), 
                [asm_addr] "r"      (addr),
                [asm_length]   "r"  (length),
                [asm_prot]   "r"    ((u64)prot),
                [asm_flags]   "r"   ((u64)flags),
                [asm_fd]   "r"      ((u64)fd),
                [asm_offset]   "r"  ((u64)offset)
            : "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "memory"
    );

    return result;
}

s32 sys_munmap(void *addr, u64 length) {
    s32 result = 0;

    __asm__ volatile (
            "   movq %[asm_addr],   %%rdi\n"
            "   movq %[asm_length], %%rsi\n"
            "   movl %[sys_call_n], %%eax\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n] "r" (SYS_CALL_MUNMAP), 
                [asm_addr]   "r" (addr),
                [asm_length] "r" (length)
            : "%rdi", "%rsi"
    );

    return result;
}

s32 sys_socket(s32 domain, s32 type, s32 protocol) {
    s32 result = 0;

    __asm__ volatile (
            "   movl %[sys_call_n], %%eax\n"
            "   movq %[asm_domain], %%rdi\n"
            "   movq %[asm_type],   %%rsi\n"
            "   movq %[asm_prot],   %%rdx\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n] "r" (SYS_CALL_SOCKET), 
                [asm_domain] "r" ((u64)domain),
                [asm_type]   "r" ((u64)type),
                [asm_prot]   "r" ((u64)protocol)
            : "%rdi", "%rsi", "%rdx"
    );

    return result;
}

s32 sys_connect(
        s32 socket_fd, 
        const SocketAddress *socket_addr, 
        u32 socket_len
) {
    s32 result = 0;

    __asm__ volatile (
            "   movl %[sys_call_n],     %%eax\n"
            "   movq %[asm_socket_fd],  %%rdi\n"
            "   movq %[asm_socket_addr],%%rsi\n"
            "   movq %[asm_socket_len], %%rdx\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n]        "r" (SYS_CALL_CONNECT),
                [asm_socket_fd]     "r" ((u64)socket_fd),
                [asm_socket_addr]   "r" ((u64)socket_addr),
                [asm_socket_len]    "r" ((u64)socket_len)
            : "%rdi", "%rsi", "%rdx"
    );
    
    return result;
}

s64 sys_sendto(
        s32 socket_fd, 
        u8 *buf,
        u64 len,
        s32 flags,
        const SocketAddress *dest_addr,
        u32 addr_len
) {
    s64 result = 0;

    __asm__ volatile (
            "   movl %[sys_call_n],     %%eax\n"
            "   movq %[asm_socket_fd],  %%rdi\n"
            "   movq %[asm_buffer],     %%rsi\n"
            "   movq %[asm_buffer_len], %%rdx\n"
            "   movq %[asm_flags],      %%r10\n"
            "   movq %[asm_dest_addr],  %%r8\n"
            "   movq %[asm_addr_len],   %%r9\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n]        "r" (SYS_CALL_SENDTO),
                [asm_socket_fd]     "r" ((u64)socket_fd),
                [asm_buffer]        "r" ((u64)buf),
                [asm_buffer_len]    "r" (len),
                [asm_flags]         "r" ((u64)flags),
                [asm_dest_addr]     "r" ((u64)dest_addr),
                [asm_addr_len]      "r" ((u64)addr_len)
            : "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9"
    );
    
    return result;
}

void sys_exit(u16 exit_code) {
    __asm__ volatile (
            "   movl %[sys_call_n],     %%eax\n"
            "   movq %[asm_exit_code],  %%rdi\n"
            "   syscall\n"
            : /* NO OUTPUT */
            :   [sys_call_n]    "r" (SYS_CALL_EXIT),
                [asm_exit_code] "r" ((u64)exit_code)
            : "%rdi"
    );
}


