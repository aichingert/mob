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

#define IS_STRING_EQL(string, slice)        \
    memeql(string.val, string.len, u8 ## slice, sizeof(slice) - 1)
#define CMP_TO_STRING(slice, string, off)   \
    memeql(u8 ## slice, sizeof(slice) - 1, string.val + off, MIN(string.len - off, sizeof(slice) - 1))

struct C_Macro {
    u8 *key;

    String args;
    String data;
};

struct Module {
    String *includes;
    C_Macro *defines;
};

bool is_ident_start(u8 character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character == '_');
}

bool is_ident(u8 character) {
    return is_ident_start(character) || (character >= '0' && character <= '9');
}

bool read_ident(u32 *out_pos, String source) {
    if (*out_pos < source.len && is_ident_start(source.val[*out_pos])) {
        *out_pos += 1;
    }

    while (*out_pos < source.len && is_ident(source.val[*out_pos])) {
        *out_pos += 1;
    }

    return *out_pos < source.len;
}

bool skip_whitespace_and_new_line(u32 *out_pos, String source) {
    while (*out_pos < source.len 
        && (source.val[*out_pos] == ' ' || source.val[*out_pos] == '\n')) {
        *out_pos += 1;
    }

    return *out_pos < source.len;
}

Module create_module_from_file(Arena *allocator, String file_name) {
    // TODO: make logging optional
    printf("[INFO] reading file: `%s`\n", file_name.val);

    Module module = {0};

    u32 i = 0;
    String source = file_read_as_string_alloc(allocator, file_name);

    while (i < source.len) {
        switch (source.val[i]) {
            case 's':
            break;
            case 't':
            break;
            case '#':
                if          (CMP_TO_STRING("#include ", source, i)) {
                    u64 beg = i;
                    while (i < source.len && source.val[i] != '\n') {
                        i += 1;
                    }

                    String include = str_copy(allocator, source, beg, i + 1);
                    array_push(allocator, module.includes, include);
                } else if   (CMP_TO_STRING("#define ", source, i)) {
                    i += sizeof("#define");
                    while (i < source.len && source.val[i] == ' ') {
                        i += 1;
                    }

                    u64 beg = i;
                    assert(read_ident(&i, source), S("failed to read define macro identifer"));
                    i += 1;

                    String ident = str_copy(allocator, source, beg, i);
                    String args  = {0};
                    while (i < source.len && source.val[i] == ' ') {
                        i += 1;
                    }

                    if (i < source.len && source.val[i] == '(') {
                        beg = i;
                        u16 braces = 1;

                        while (i < source.len && braces > 0) {
                            if          (source.val[i] == '(') {
                                braces += 1;
                            } else if   (source.val[i] == ')') {
                                braces -= 1;
                            }

                            i += 1;
                        }

                        args = str_copy(allocator, source, beg, i);
                    }

                    beg = 0;
                    u32 bks = 0;
                    while (i < source.len && (source.val[i] != '\n' || bks > 0)) {
                        if          (source.val[i] == '\n') {
                            bks -= 1;
                        } else if   (source.val[i] == '\\') {
                            bks += 1;
                        }
                        i += 1;
                    }

                    C_Macro define = {
                        .args = args,
                        .data = str_copy(allocator, source, beg, i + 1),
                    };
                    hm_put_ptr(allocator, module.defines, ident.val, ident.len, define);
                }
            break;
        }

        i += 1;
    }

    return module;
}

// TODO: add usage only file globals supported 
// unless adding it to the context which will
// be generated in the unit file so env does not
// have to be declared by each program
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
        printf("%d includes\n", array_len(mod.includes));
        for (u32 i = 0; i < array_len(mod.includes); i++) {
            for (u32 j = 0; j < mod.includes[i].len; j++) {
                printf("%c", mod.includes[i].val[j]);
            }
        }
        printf("\n");
    }

    

    arena_deinit(&allocator);
    return 0;
}

