// NOTE: https://gaultier.github.io/blog/wayland_from_scratch.html
// NOTE: only supporting wayland

typedef struct Window {
    u16 width;
    u16 height;
} Window;

u32 wayland_current_id = 1;

static const u32 wayland_display_object_id = 1;
static const u16 wayland_wl_registry_event_global = 0;
static const u16 wayland_shm_pool_event_format = 0;
static const u16 wayland_wl_buffer_event_release = 0;
static const u16 wayland_xdg_wm_base_event_ping = 0;
static const u16 wayland_xdg_toplevel_event_configure = 0;
static const u16 wayland_xdg_toplevel_event_close = 1;
static const u16 wayland_xdg_surface_event_configure = 0;
static const u16 wayland_wl_display_get_registry_opcode = 1;
static const u16 wayland_wl_registry_bind_opcode = 0;
static const u16 wayland_wl_compositor_create_surface_opcode = 0;
static const u16 wayland_xdg_wm_base_pong_opcode = 3;
static const u16 wayland_xdg_surface_ack_configure_opcode = 4;
static const u16 wayland_wl_shm_create_pool_opcode = 0;
static const u16 wayland_xdg_wm_base_get_xdg_surface_opcode = 2;
static const u16 wayland_wl_shm_pool_create_buffer_opcode = 0;
static const u16 wayland_wl_surface_attach_opcode = 1;
static const u16 wayland_xdg_surface_get_toplevel_opcode = 1;
static const u16 wayland_wl_surface_commit_opcode = 6;
static const u16 wayland_wl_display_error_event = 0;
static const u32 wayland_format_xrgb8888 = 1;
static const u32 wayland_header_size = 8;
static const u32 color_channels = 4;

void read_wayland_env(
        String xdg_runtime_dir_name, 
        String *xdg_runtime_dir, 
        String wayland_display_name, 
        String *wayland_display
) {
    char **env_ptr = ENV;

    while (*env_ptr != NULL 
            && (xdg_runtime_dir->len == 0 || wayland_display->len == 0)) {
        if          (c_string_begins_with(*env_ptr, xdg_runtime_dir_name)) {
            *xdg_runtime_dir = from_c_string(
                    *env_ptr + xdg_runtime_dir_name.len + 1);
        } else if   (c_string_begins_with(*env_ptr, wayland_display_name)) {
            *wayland_display = from_c_string(
                    *env_ptr + wayland_display_name.len + 1);
        }

        env_ptr += 1;
    }
}

s32 wayland_display_connect() {
    String xdg_runtime_dir = {0};
    String wayland_display = {0};
    read_wayland_env(
            S("XDG_RUNTIME_DIR"), 
            &xdg_runtime_dir, 
            S("WAYLAND_DISPLAY"), 
            &wayland_display);
    assert(xdg_runtime_dir.val != NULL, "no xdg runtime dir set");

    if (wayland_display.val == NULL) {
        wayland_display = S("wayland-0");
    }

    UnixSocketAddress addr = { .socket_family = AF_UNIX };
    assert(
            xdg_runtime_dir.len + wayland_display.len + 1 < UNIX_PATH_MAX,
            "wayland socket path does not fit");
    memcpy(addr.socket_path, xdg_runtime_dir.val, xdg_runtime_dir.len);
    addr.socket_path[xdg_runtime_dir.len] = '/';
    memcpy(
            addr.socket_path + xdg_runtime_dir.len + 1, 
            wayland_display.val, wayland_display.len);

    s32 fd = sys_socket(AF_UNIX, SOCK_STREAM, 0);
    assert(fd != -1, "unable to create socket");
    s32 result = sys_connect(fd, (SocketAddress*)&addr, sizeof(addr));
    assert(result != -1, "unable to connect to wayland socket");

    return fd;
}

 #define roundup_4(n) (((n) + 3) & -4)

u32 wayland_display_get_registry(s32 fd) {
    u64 size = 0;
    u8 buf[128] = {0};

    mem_write_u32(buf, &size, sizeof(buf), wayland_display_object_id);
    mem_write_u16(
            buf, 
            &size, 
            sizeof(buf), 
            wayland_wl_display_get_registry_opcode);
    u16 msg_announced_size = wayland_header_size + sizeof(wayland_current_id);
    assert(
            roundup_4(msg_announced_size) == msg_announced_size, 
            "invalid msg size");
    mem_write_u16(
            buf, 
            &size, 
            sizeof(buf), 
            msg_announced_size);

    wayland_current_id += 1;
    mem_write_u32(buf, &size, sizeof(buf), wayland_current_id);

    assert(((s64)size != sys_sendto(fd, buf, size, MSG_DONTWAIT, NULL, 0)), "send failed");

    printf(
            "-> wl_display@%u.get_registry: wl_registry=%u\n",
            wayland_display_object_id, 
            wayland_current_id);
    return wayland_current_id;
}

Window create_window(u16 width, u16 height) {
    s32 fd = wayland_display_connect();
    u32 wl_registry = wayland_display_get_registry(fd);

    return (Window){
        .width = width,
        .height = height,
    };
}
