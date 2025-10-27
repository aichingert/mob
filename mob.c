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
static const u32    PATH_LEN = sizeof(PATHS) / sizeof(PATHS[0]);

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

    TT_L_BRACE      = 40,
    TT_R_BRACE      = 41,
    TT_L_PAREN      = 42,
    TT_R_PAREN      = 43,
    TT_SEMICOLON    = 47,
    TT_BACKSLASH    = 48,

    M_PRIORITY_T    = 60,
    M_PRIORITY_F    = 60,

    R_EOF           = 80,
};

struct Token {
    u32 beg;
    u32 line;
    TokenType type;
};

struct Tokens {
    __ARRAY_HEADER__;
    Token *val;
};

void tokenize(Arena *stack, u8 *source, s64 size) {
    u32 pos = 0;

    Tokens toks = {0};
    push(stack, &toks, (Token){ .beg = 10 });
    push(stack, &toks, (Token){ .beg = 20 });
    push(stack, &toks, (Token){ .beg = 30 });

    for (u64 i = 0; i < toks.len; i++) {
        printf("%u\n", toks.val[i].beg);
    }

    while (pos < size) {

        switch (source[pos]) {
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
        }

        pos += 1;
    }

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

        Buffer src = file_read_as_string_alloc(&app, PATHS[i]);
        tokenize(&app, src.mem, src.len);

    }

    arena_deinit(&app);
    return 0;
}

