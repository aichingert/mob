#define SYS_CALL_SOCKET     41
#define SYS_CALL_CONNECT    42
#define SYS_CALL_SENDTO     44
#define SYS_CALL_EXIT       60

void assert(bool condition, char *msg) {
    #ifndef DEBUG

    if (!condition) {
        printf("ERROR: %s\n", msg);
        sys_exit(1);
    }

    #endif
}

s32 sys_socket(s32 domain, s32 type, s32 protocol) {
    u64 result = 0;

    __asm__ volatile (
            "   movq %[sys_call_n], %%rax\n"
            "   movq %[asm_domain], %%rdi\n"
            "   movq %[asm_type],   %%rsi\n"
            "   movq %[asm_prot],   %%rdx\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n] "r" ((u64)SYS_CALL_SOCKET), 
                [asm_domain] "r" ((u64)domain),
                [asm_type]   "r" ((u64)type),
                [asm_prot]   "r" ((u64)protocol)
            : "%rdi", "%rsi", "%rdx"
    );

    return (s32)result;
}

s32 sys_connect(
        s32 socket_fd, 
        const SocketAddress *socket_addr, 
        u32 socket_len
) {
    u64 result = 0;

    __asm__ volatile (
            "   movq %[sys_call_n],     %%rax\n"
            "   movq %[asm_socket_fd],  %%rdi\n"
            "   movq %[asm_socket_addr],%%rsi\n"
            "   movq %[asm_socket_len], %%rdx\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n]        "r" ((u64)SYS_CALL_CONNECT),
                [asm_socket_fd]     "r" ((u64)socket_fd),
                [asm_socket_addr]   "r" ((u64)socket_addr),
                [asm_socket_len]    "r" ((u64)socket_len)
            : "%rdi", "%rsi", "%rdx"
    );
    
    return (s32)result;
}

u64 sys_sendto(
        s32 socket_fd, 
        u8 *buf,
        u64 len,
        s32 flags,
        const SocketAddress *dest_addr,
        u32 addr_len
) {
    u64 result = 0;

    __asm__ volatile (
            "   movq %[sys_call_n],     %%rax\n"
            "   movq %[asm_socket_fd],  %%rdi\n"
            "   movq %[asm_buffer],     %%rsi\n"
            "   movq %[asm_buffer_len], %%rdx\n"
            "   movq %[asm_flags],      %%r10\n"
            "   movq %[asm_dest_addr],  %%r8\n"
            "   movq %[asm_addr_len],   %%r9\n"
            "   syscall\n"
            : "=r" (result)
            :   [sys_call_n]        "r" ((u64)SYS_CALL_SENDTO),
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
            "   movq %[sys_call_n],     %%rax\n"
            "   movq %[asm_exit_code],  %%rdi\n"
            "   syscall\n"
            : /* NO OUTPUT */
            :   [sys_call_n]    "r" ((u64)SYS_CALL_EXIT),
                [asm_exit_code] "r" ((u64)exit_code)
            : "%rdi"
    );
}


