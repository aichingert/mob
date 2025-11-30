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

enum FieldType {
    MOB_IDENT,
    MOB_MACRO,
};

struct C_MacroEmbedStruct {
    String ident;
    String *args;
    String *fields;
};

struct C_MacroCall {
    String ident;
    String *args;
};

struct Field {
    FieldType field_type;

    union {
        String      ident;
        C_MacroCall call;
    };
};

struct Range {
    u32 beg;
    u32 end;
};

struct Plex {
    String ident;
    Field *fields;
};

struct Func {

};

struct Module {

    C_MacroEmbedStruct *macros;
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

void parse_plex_fields(Arena *allocator, u32 *pos, String source, Plex *out_plex) {
    while (*pos < source.len && source.val[*pos] != '}') {
        assert(skip_whitespace_and_new_line(pos, source), S("unexpected eof while reading struct fields"));

        u32 beg = *pos;
        assert(read_ident(pos, source), S("expected one of 'type', 'ident' or 'macro' in struct field start"));
        String str = {
            .val = source.val + beg,
            .len = *pos - beg,
        };
        assert(skip_whitespace_and_new_line(pos, source), S("unexpected eof while declaring struct"));

        if          (IS_STRING_EQL(str, "struct") || IS_STRING_EQL(str, "union")) {
            assert(source.val[*pos] == '{', S("expected '{' after nested struct or union"));
            *pos += 1;

            parse_plex_fields(allocator, pos, source, out_plex);
        } else if   (source.val[*pos] == ';') {
            Field mf = {
                .field_type = MOB_MACRO,
                .call       = {
                    .ident = str,
                },
            };

            array_push(allocator, out_plex->fields, mf);
        } else if   (source.val[*pos] == '(') {
            if (*pos + 1 < source.len && source.val[*pos + 1] == '*') {
                // SKIP: function pointers that return 
                // not fully known types are resolved later
                for (; *pos < source.len && source.val[*pos] != ';'; (*pos)++);
                assert(*pos + 1 <= source.len, S("unexpected eof after function pointer in struct"));
                *pos += 1;
            } else {
                // NOTE: this a macro with arguments check
            }
        }


        printf("%u %u %u\n", beg, *pos, source.len);

        printf("%u - %c\n", *pos, source.val[*pos]);
    }

    *pos += 1;
    assert(*pos + 1 < source.len && source.val[*pos] == ';', S("unexpected eof while reading struct fields"));
    *pos += 1;
}

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

                plx.ident.val = source.val + i;
                assert(read_ident(&i, source), S("missing ident after struct keyword"));
                plx.ident.len = i - plx.ident.len;

                assert(skip_whitespace_and_new_line(&i, source), S("unexpected eof after struct ident"));
                assert(source.val[i] == '{', S("expected '{' after struct ident"));
                i += 1;

                parse_plex_fields(allocator, &i, source, &plx);

            continue;
            case 't':
                i += 1;
                continue;
            case '#':
                // TODO: extract macros
                i += 1;
            continue;
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
    }

    arena_deinit(&allocator);
    return 0;
}

