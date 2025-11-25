#include <stdio.h>

char **ENV = NULL;

static const String BUILD = S("app.c");
static const String FLAGS[] = {
    S("-std=c23"),
    S("-ffreestanding"),
    S("-Wextra"),
    S("-Wall"),
};
static const String PATHS[] = {
    S("example/app.c"),
    S("example/math.c"),
};
static const u32    PATH_LEN = mob_static_array_len(PATHS);

#define CMP_TO_STRING(slice, string, off) \
    memeql(u8 ## slice, sizeof(slice) - 1, string.val + off, MIN(string.len - off, sizeof(slice) - 1))

struct Plex {
    String ident;

};
struct Func {

};
struct Module {

};

bool is_ident_start(u8 character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z');
}

bool is_ident(u8 character) {
    return (is_ident_start(character))
        || (character == '_')
        || (character >= '0' && character <= '9');
}

bool read_ident(u32 *out_len, String source) {
    if (*out_len < source.len && is_ident_start(source.val[*out_len])) {
        *out_len += 1;
    }

    while (*out_len < source.len && is_ident(source.val[*out_len])) {
        *out_len += 1;
    }

    return *out_len < source.len;
}

bool skip_whitespace_and_new_line(u32 *out_len, String source) {
    while (*out_len < source.len 
        && (source.val[*out_len] == ' ' || source.val[*out_len] == '\n')) {
        *out_len += 1;
    }

    return *out_len < source.len;
}

// TODO: think about handling globals....
// how can they be modeled with this idea
Module create_module_from_file(Arena *allocator, String file_name) {
    printf("[INFO] reading file: `%s`\n", file_name.val);

    u32 i = 0;
    Module module = {0};
    String source = file_read_as_string_alloc(allocator, file_name);

    while (i < source.len) {
        switch (source.val[i]) {
            case 's':
                Plex plx = {0};

                if (!CMP_TO_STRING("struct ", source, i)) {
                    break;
                }
                i += sizeof("struct"); // NOTE: using \0 as whitespace
                assert(skip_whitespace_and_new_line(&i, source), S("unexpected eof after struct"));
                u64 beg = i;
                plx.ident.val = source.val;
                plx.ident.len = i;
                assert(read_ident(&i, source), S("invalid ident after struct"));
                plx.ident.len = i - plx.ident.len;

                // TODO: read remaining fields of struct
                // maybe in hashset... not sure

            continue;
            case 't':
                i += 1;
            continue;
        }

        i += 1;
    }

    return module;
}

s32 main(s32 argc, const char **argv, char **environ) {
    // TODO: maybe add flags
    (void)argc;
    (void)argv;
    ENV = environ;
    Arena allocator = {0};
    arena_init(&allocator, 2 << 24);

    for (u32 i = 0; i < PATH_LEN; i++) {
        // NOTE: used to be able to distribute 
        // module creation across multiple threads
        Module mod = create_module_from_file(&allocator, PATHS[i]);
    }

    arena_deinit(&allocator);
    return 0;
}

