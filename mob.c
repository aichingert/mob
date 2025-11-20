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
    C_ENDIF         = 25,
    C_ERROR         = 27,
    C_PRAGMA        = 28,
    C_DEFINE        = 29,
    C_INCLUDE       = 30,
    C_BODY          = 31,

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

struct Module {

    StringBuilder m_includes;
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

bool read_whitespace_and_newline(u32 *pos, u32 *line, String source) {
    while (*pos < source.len && (source.val[*pos] == ' ' || source.val[*pos] == '\n')) {
        if (source.val[*pos] == '\n') {
            *line += 1;
        }
        *pos += 1;
    }
    return *pos < source.len;
}

bool read_until_newline_or_backslash(u32 *pos, u32 *line, String source) {
    while (*pos < source.len && source.val[*pos] != '\n') {
        if          (source.val[*pos] == '\n') {
            *line += 1;
        } else if   (source.val[*pos] == '\\') {
            *pos += 1;

            assert(
                    read_whitespace_and_newline(pos, line, source),
                    S("unexpected eof after backslash in macro"));
            assert(
                    source.val[*pos] == '\n',
                    S("expected new line after backslash in macro"));
        }

        *pos += 1;
    }

    return *pos < source.len;
}

bool read_until_either(u32 *pos, u32 *line, String source, String *options, u32 option_size) {
    while (*pos < source.len) {
        for (u32 i = 0; i < option_size; i++) {
            String view = {
                .val = source.val + *pos,
                .len = source.len - *pos,
            };

            if (source.val[*pos] == '\n') {
                *line += 1;
            }
            if (str_begins_with(view, options[i])) {
                *pos += options[i].len;
                return true;
            }
        }

        *pos += 1;
    }

    return false;
}

Token *tokenize(Arena *stack, String source) {
    u32 pos = 0;
    u32 line = 0;
    Token *toks = NULL;

    while (pos < source.len) {
        Token tok = { .beg = pos, .line = line, .type = C_INCLUDE };

        if (is_identifier_start(source.val[pos])) {
            u32 len = read_identifier(&pos, source);

            if          (CMP_STR("typedef", source.val + pos - len, len)) {
                tok.type = T_TYPEDEF;
            } else if   (CMP_STR("struct", source.val + pos - len, len)) {
                tok.type = T_STRUCT;
            } else {
                tok.type = T_IDENT;
            }

            array_push(stack, toks, tok);
            continue;
        }

        switch (source.val[pos]) {
            case '(':
                tok.type = TT_L_PAREN;
            break;
            case ')':
                tok.type = TT_R_PAREN;
            break;
            case '{':
                tok.type = TT_L_BRACE;
            break;
            case '}':
                tok.type = TT_R_BRACE;
            break;
            case '#':
                pos += 1;
                u32 len = read_identifier(&pos, source) + 1;

                if          (CMP_STR("#include", source.val + pos - len, len)) {
                    assert(
                        read_whitespace_and_newline(&pos, &line, source), 
                        S("unexpected eof after include"));
                    String end[] = {S("'"), S(">")};

                    assert(
                        (source.val[pos] == '"' || source.val[pos] == '<'),
                        S("invalid include start expected '\"' or '<'"));
                    assert(
                        read_until_either(&pos, &line, source, end, mob_static_array_len(end)),
                        S("include end not found"));
                } else {
                    // TODO: tokenize if preprocessor
                    if   (CMP_STR("#define", source.val + pos - len, len)) {
                        tok.type = C_DEFINE;
                    } else if   (CMP_STR("#if", source.val + pos - len, len)) {
                        tok.type = C_IF;
                    } else if   (CMP_STR("#ifdef", source.val + pos - len, len)) {
                        tok.type = C_IFDEF;
                    } else if   (CMP_STR("#ifndef", source.val + pos - len, len)) {
                        tok.type = C_IFNDEF;
                    } else if   (CMP_STR("#elif", source.val + pos - len, len)) {
                        tok.type = C_ELIF;
                    } else if   (CMP_STR("#else", source.val + pos - len, len)) {
                        tok.type = C_ELSE;
                    } else if   (CMP_STR("#endif", source.val + pos - len, len)) {
                        tok.type = C_ENDIF;
                    } else if   (CMP_STR("#undef", source.val + pos - len, len)) {
                        assert(false, S("cannot use undef with mob"));
                    } else {
                        printf("unexpected: '");
                        for (u32 i = pos - len; i < pos; i++) {
                            printf("%c", source.val[i]);
                        }
                        printf("'\n");
                        assert(false, S("expected c preprocessor macro"));
                    }

                    assert(
                        read_until_newline_or_backslash(&pos, &line, source), 
                        S("unexpected eof in macro definition"));
                }
                pos -= 1;
            break;
            case '\n': line += 1; [[ fallthrough ]];
            default: 
                pos += 1;
                continue;
        }

        pos += 1;
        array_push(stack, toks, tok);
    }

    return toks;
}

void append_to_module(Arena *app, Module *module, Token *toks) {
    (void)app;
    (void)module;

    for (u64 i = 0; i < mob_array_len(toks); i++) {

    }

}

// NOTE: implementation decided
// this preprocessor is just a 
// type finder and will put all 
// function definitions on top
// but lets the single files be 
// and just includes them as the
// final project therefore you
// get the positives of multithread
// builds

struct TestMap {
    s32 key;

    u8 val;
    u32 file_size;
};

s32 main(s32 argc, const char **argv, char **environ) {
    // TODO: maybe add flags
    (void)argc;
    (void)argv;
    ENV = environ;
    Arena mob = {0};
    arena_init(&mob, 2 << 20);

    String s = S("hello, world\n");
    String s1 = S("hello,world\n");
    u64 mod = 4096;

    TestMap *map = NULL;
    TestMap values = { .val = 10, .file_size = 4000 };
    s32 key = -1;
    char a = 'C';
    mob_hm_put(&mob, map, key, values);
    key = 100;
    a = 'B';
    mob_hm_put(&mob, map, key, a);
    key = -1;
    TestMap *res = mob_hm_get(&mob, map, key);

    printf("---------------\n");
    printf("OUT: %d %d - %u\n", res->key, res->val, res->file_size);
    printf("---------------\n");

    printf("%lu - %lu\n", mob_hm_hasher(s.val, 1, s.len) % mod, mob_hm_hasher(s1.val, 1, s1.len) % mod);

    Module module = {0};

    for (u32 i = 0; i < PATH_LEN; i++) {
        printf("[INFO] reading file: `%s`\n", PATHS[i].val);

        String vals = file_read_as_string_alloc(&mob, PATHS[i]);
        Token *toks = tokenize(&mob, vals);
        append_to_module(&mob, &module, toks);
    }

    arena_deinit(&mob);
    return 0;
}

