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

static bool is_log_enabled = false;

#define IS_STRING_EQL(string, slice)        \
    memeql(string.val, string.len, u8 ## slice, sizeof(slice) - 1)
#define CMP_TO_STRING(slice, string, off)   \
    memeql(u8 ## slice, sizeof(slice) - 1, string.val + off, MIN(string.len - off, sizeof(slice) - 1))

struct C_Struct {
    String name;
    String data;
};

struct C_Macro {
    String name;
    String args;
    String data;
};

// TODO: use str_hm for 
// structs and defines
struct Module {
    String *raw_copy;
    C_Macro *defines;
    C_Struct *structs;
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

void create_module_from_file(Arena *allocator, String file_name, Module *out_mod) {
    if (is_log_enabled) {
        printf("[INFO] reading file: `%s`\n", file_name.val);
    }

    u32 i = 0;
    String source = file_read_as_string_alloc(allocator, file_name);

    while (i < source.len) {
        u64 beg;

        switch (source.val[i]) {
            case 's':
                if (!CMP_TO_STRING("struct ", source, i)) {
                    break;
                }
                i += sizeof("struct"); // '\0'
                assert(skip_whitespace_and_new_line(&i, source), S("expected struct ident but got eof"));

                beg = i;
                assert(read_ident(&i, source), S("failed to read struct ident"));

                String name = str_copy(allocator, source, beg, i + 1);
                assert(skip_whitespace_and_new_line(&i, source), S("expected struct body but got eof"));
                assert(source.val[i] == '{', S("expected opening brace after struct ident"));
                u16 braces = 1;
                beg = i;
                i += 1;

                while (i < source.len && braces > 0) {
                    if          (source.val[i] == '{') {
                        braces += 1;
                    } else if   (source.val[i] == '}') {
                        braces -= 1;
                    }

                    i += 1;
                }

                C_Struct plex = {
                    .name = name,
                    .data = str_copy(allocator, source, beg, i),
                };
                array_push(allocator, out_mod->structs, plex);
            break;
            case 't':
                if (!CMP_TO_STRING("typedef ", source, i)) {
                    break;
                }
                beg = i;
                i += sizeof("typedef");
                assert(skip_whitespace_and_new_line(&i, source), S("unexpected eof after typedef"));


                if (CMP_TO_STRING("struct ", source, i)) {
                    i += sizeof("struct");
                    while (i < source.len && source.val[i] != ';' && source.val[i] != '{') {
                        i += 1;
                    }

                    if (i < source.len && source.val[i] == '{') {
                        i = beg + sizeof("typedef");
                        break;
                    }
                }
                while (i < source.len && source.val[i] != ';') {
                    i += 1;
                }

                String c_typedef = str_copy(allocator, source, beg, i + 1);
                array_push(allocator, out_mod->raw_copy, c_typedef);
            break;
            case '#':
                if          (CMP_TO_STRING("#include ", source, i)) {
                    beg = i;
                    while (i < source.len && source.val[i] != '\n') {
                        i += 1;
                    }

                    String include = str_copy(allocator, source, beg, i);
                    array_push(allocator, out_mod->raw_copy, include);
                } else if   (CMP_TO_STRING("#define ", source, i)) {
                    i += sizeof("#define");
                    while (i < source.len && source.val[i] == ' ') {
                        i += 1;
                    }

                    beg = i;
                    assert(read_ident(&i, source), S("failed to read define macro identifer"));
                    i += 1;

                    String name = str_copy(allocator, source, beg, i);
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
                        .name = name,
                        .args = args,
                        .data = str_copy(allocator, source, beg, i + 1),
                    };
                    array_push(allocator, out_mod->defines, define);
                }
            break;
            case '(':
                u32 tmp = i;

                while (tmp < source.len && source.val[tmp] != ')') {
                    tmp += 1;
                }
                skip_whitespace_and_new_line(&tmp, source);
                if (tmp < source.len && source.val[tmp] == '{') {
                    s64 pos = i;
                    u8 spce = 0;

                    while (pos > 0 && spce <= 1) {
                        if (source.val[pos] == ' ') {
                            spce += 1;
                        }
                        pos -= 1;
                    }

                    tmp = 0;
                    // go back until two ident
                }
            break;
        }

        i += 1;
    }
}

// TODO: add usage only file globals supported 
// unless adding it to the context which will
// be generated in the unit file so env does not
// have to be declared by each program
s32 main(s32 argc, const char **argv, char **environ) {
    ENV = environ;

    if (argc > 1) {
        String logging_flag = S("--enable_log");
        String str_argument = from_c_string((char*)argv[1]);

        if (memeql(logging_flag.val, logging_flag.len, str_argument.val, str_argument.len)) {
            is_log_enabled = true;
        }
    }

    Arena allocator = {0};
    arena_init(&allocator, 2 << 24);

    Module module = {0};
    for (u32 i = 0; i < PATH_LEN; i++) {
        create_module_from_file(&allocator, PATHS[i], &module);
    }

    printf("%d includes\n", array_len(module.raw_copy));
    for (u32 i = 0; i < array_len(module.raw_copy); i++) {
        for (u32 j = 0; j < module.raw_copy[i].len; j++) {
            printf("%c", module.raw_copy[i].val[j]);
        }
        printf("\n");
    }

    printf("%d structs\n", array_len(module.structs));
    for (u32 i = 0; i < array_len(module.structs); i++) {
        printf("NAME: \n");
        for (u32 j = 0; j < module.structs[i].name.len; j++) {
            printf("%c", module.structs[i].name.val[j]);
        }
        printf("\n");
        printf("DATA\n");
        for (u32 j = 0; j < module.structs[i].data.len; j++) {
            printf("%c", module.structs[i].data.val[j]);
        }
        printf("\n");
    }

    arena_deinit(&allocator);
    return 0;
}

