#define UNIX_PATH_MAX 108

#define AF_UNIX 1

#define SOCK_STREAM 2

#define MSG_DONTWAIT    0x40
#define MSG_EOR         0x80
#define MSG_WAITALL     0x100

struct SocketAddress {
    u16 socket_family;
    s8  socket_data[14];
};

struct UnixSocketAddress {
    u16 socket_family;
    s8  socket_path[UNIX_PATH_MAX];
};
