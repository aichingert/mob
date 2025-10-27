void file_read_as_string(Buffer buf, String name) {
    s32 file = os_open_file(name, O_RDONLY, 644);
    assert(file > 0, S("unable to open file"));

    s64 size = os_file_size(file);
    assert((s64)buf.len > size, S("file bigger than provided buffer"));
    assert(os_read(file, buf.mem, size) > 0, S("unable to read file"));

    os_close_file(file);
}

Buffer file_read_as_string_alloc(Arena *arena, String name) {
    s32 file = os_open_file(name, O_RDWR, 644);
    assert(file > 0, S("unable to open file"));

    s64 size = os_file_size(file);
    u8 *mem = alloc(arena, u8, size);
    assert(os_read(file, mem, size) > 0, S("unable to read file"));

    os_close_file(file);
    return (Buffer){
        .mem = mem,
        .len = size,
    };
}
