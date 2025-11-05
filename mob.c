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
static const u32    PATH_LEN = __ARRAY_LEN(PATHS);

#define CMP_STR(str, src, src_len) memeql(u8 ## str, sizeof(str) - 1, src, src_len)

enum TokenType {
    T_ENUM          = 0,
    T_STRUCT        = 1,
    T_TYPEDEF       = 2,
    T_IDENT         = 3,
    T_NUMBER        = 4,

    C_IF            = 20,
    C_IFDEF         = 21,
    C_IFNDEF        = 22,
    C_ELSE          = 23,
    C_ELIF          = 24,
    C_UNDEF         = 25,
    C_ERROR         = 27,
    C_PRAGMA        = 28,
    C_DEFINE        = 29,
    C_INCLUDE       = 30,
    C_MACRO_CONCAT  = 31,
    C_BODY          = 32,

    TT_L_BRACE      = 40,
    TT_R_BRACE      = 41,
    TT_L_PAREN      = 42,
    TT_R_PAREN      = 43,
    TT_SEMICOLON    = 47,
    TT_BACKSLASH    = 48,

    M_PRIORITY_T    = 60,

    R_EOF           = 80,
};

struct Token {
    u32 beg;
    u32 line;
    TokenType type;
};

struct Tokens {
    __ARRAY_HEADER__;
    Token *arr;
};

bool is_identifier_start(u8 character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z');
}

bool is_identifier(u8 character) {
    return is_identifier_start(character)
        || character == '_'
        || (character >= '0' && character <= '9');
}

u32 read_identifier(u32 *pos, String source) {
    u32 beg = *pos;
    while (*pos < source.len && is_identifier(source.val[*pos])) {
        *pos += 1;
    }
    return *pos - beg;
}

bool read_whitespace(u32 *pos, String source) {
    while (*pos < source.len && source.val[*pos] == ' ') {
        *pos += 1;
    }
    return *pos < source.len;
}

bool read_until_either(u32 *pos, String source, String *options, u32 option_size) {
    while (*pos < source.len) {
        for (u32 i = 0; i < option_size; i++) {
            String view = {
                .val = source.val + *pos,
                .len = source.len - *pos,
            };

            if (str_begins_with(view, options[i])) {
                *pos += 1;
                return true;
            }
        }

        *pos += 1;
    }

    return false;
}

Tokens tokenize(Arena *stack, String source) {
    u32 pos = 0;
    u32 line = 0;
    Tokens toks = {0};

    while (pos < source.len) {
        if (is_identifier_start(source.val[pos])) {
            u32 len = read_identifier(&pos, source);
            array_push(stack, &toks, ((Token){ .beg = pos - len, .line = line }));
            continue;
        }

        switch (source.val[pos]) {
            case '(':
                printf("(\n");
            break;
            case ')':
                printf(")\n");
            break;
            case '{':
                printf("{\n");
            break;
            case '}':
                printf("}\n");
            break;
            case '#':
                Token tok = { .beg = pos, .line = line, .type = C_INCLUDE };
                array_push(stack, &toks, tok);
                pos += 1;
                u32 len = read_identifier(&pos, source) + 1;

                if          (CMP_STR("#include", source.val + pos - len, len)) {
                    assert(
                            read_whitespace(&pos, source), 
                            S("unexpected end of file after include"));
                    String end[] = {S("'"), S(">")};

                    assert(
                            (source.val[pos] == '"' || source.val[pos] == '<'),
                            S("invalid include start expected '\"' or '<'"));
                    assert(
                            read_until_either(&pos, source, end, __ARRAY_LEN(end)),
                            S("include end not found"));
                } else if   (CMP_STR("#define", source.val + pos - len, len)
                          || CMP_STR("#undef", source.val + pos - len, len)
                          || CMP_STR("#ifndef", source.val + pos - len, len)
                          || CMP_STR("#if", source.val + pos - len, len)
                          || CMP_STR("#undef", source.val + pos - len, len)
                          || CMP_STR("#elif", source.val + pos - len, len)) {

                    toks.arr[toks.len - 1].type = C_DEFINE;

                } else if (CMP_STR("#else", source.val + pos - len, len)
                        || CMP_STR("#endif", source.val + pos - len, len)) {

                } else {
                    printf("unexpected: '");
                    for (u32 i = pos - len; i < pos; i++) {
                        printf("%c", source.val[i]);
                    }
                    printf("'\n");
                    assert(false, S("expected c preprocessor macro"));
                }
            break;
        }

        pos += 1;
    }

    return toks;
}

s32 main(s32 argc, const char **argv, char **environ) {
    // TODO: maybe add flags
    (void)argc;
    (void)argv;
    ENV = environ;
    Arena app = {0};
    arena_init(&app, 2 << 20);

    for (u32 i = 0; i < PATH_LEN; i++) {
        printf("[INFO] reading file: `%s`\n", PATHS[i].val);

        String vals = file_read_as_string_alloc(&app, PATHS[i]);
        Tokens toks = tokenize(&app, vals);

        for (u64 i = 0; i < toks.len; i++) {
            if (toks.arr[i].type == C_INCLUDE) {
                printf("#include\n");
            }
        }
    }

    arena_deinit(&app);
    return 0;
}

