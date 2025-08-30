#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#define UNIT_PATH "goc_unit.c"

static const char *PATHS[] = {
    "src/goc.c",
    "src/arena.c",
    "src/arena.h",
    "src/tokenize.c",
    "src/tokenize.h",
    "src/parser.c",
    "src/parser.h",
    "src/writer.h",
    "src/writer.c",
};
static const uint32_t PATH_COUNT = sizeof(PATHS) / sizeof(PATHS[0]);

typedef struct Arena {
    char *beg;
    char *end;
} Arena;

#define alloc(...)            allocx(__VA_ARGS__,alloc4,alloc3,alloc2)(__VA_ARGS__)
#define allocx(a,b,c,d,e,...) e
#define alloc2(a, t)          (t *)arena_alloc(a, sizeof(t), alignof(t), 1, true)
#define alloc3(a, t, n)       (t *)arena_alloc(a, sizeof(t), alignof(t), n, true)
#define alloc4(a, t, n, b)    (t *)arena_alloc(a, sizeof(t), alignof(t), n, b)

#define sizeof(x)    (ptrdiff_t)sizeof(x)
#define countof(a)   (sizeof(a) / sizeof(*(a)))
#define len(s)  (countof(s) - 1)

#define push(s, arena) \
    ((s)->len >= (s)->cap \
        ? grow(s, sizeof(*(s)->data), arena), \
          (s)->data + (s)->len++ \
        : (s)->data + (s)->len++)

void *arena_alloc(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, bool zero);
void grow(void *slice, ptrdiff_t size, Arena *a);

typedef enum TokenType {
    T_ENUM      = 0,
    T_STRUCT    = 1,
    T_TYPEDEF   = 2,
    T_IDENT     = 3,

    C_IF        = 20,
    C_IFDEF     = 21,
    C_IFNDEF    = 22,
    C_ELSE      = 23,
    C_ELIF      = 24,
    C_ENDIF     = 25,
    C_UNDEF     = 26,
    C_ERROR     = 27,
    C_PRAGMA    = 28,
    C_DEFINE    = 29,
    C_INCLUDE   = 30,

    O_EQ        = 40,

    TT_L_BRACE  = 60,
    TT_R_BRACE  = 61,
    TT_L_PAREN  = 62,
    TT_R_PAREN  = 63,
    TT_L_BRACKET= 64,
    TT_R_BRACKET= 65,
    TT_COMMA    = 66,
    TT_SEMICOLON= 67,
    TT_BACKSLASH= 68,

    R_EOF       = 80,
    R_IGNORE    = 81,
} TokenType;

typedef struct Token {
    uint32_t    beg;
    uint32_t    line;
    TokenType   type;
} Token;

typedef struct ArrayToken {
    Token *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayToken;

bool is_ident(char character);
ArrayToken tokenize(Arena *arena, const char const *path, const char *source, uint32_t len);

typedef struct LineRange {
    Token start;
    uint32_t end_line;
} LineRange;

typedef struct CharRange {
    Token start;
    Token end_char;
} CharRange;

typedef struct ArrayLineRange {
    LineRange *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayLineRange;

typedef struct ArrayCharRange {
    CharRange *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayCharRange;

typedef struct FileContent {
    ArrayLineRange pragmas;
    ArrayLineRange defines;
    ArrayLineRange includes;
    ArrayLineRange compiler_ifs;

    ArrayCharRange enums;
    ArrayCharRange structs;
    ArrayCharRange globals;
    ArrayCharRange typedefs;
    ArrayCharRange functions;
} FileContent;

typedef struct ArrayFileContent {
    FileContent *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayFileContent;

typedef struct ArrayCharPtr {
    char **data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayCharPtr;

FileContent parse_c_file(Arena *arena, Arena *scratch, const char *path);

typedef struct StringBuilder {
    char *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} StringBuilder;

void write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

int main(int argc, char **argv);
void *arena_alloc(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, bool zero);
void grow(void *slice, ptrdiff_t size, Arena *a);
bool is_ident_start(char character);
bool is_ident(char character);
bool is_line_comment(uint32_t pos, uint32_t len, const char *source);
bool is_multiline_comment(uint32_t pos, uint32_t len, const char *source);
void consume_line_comment(uint32_t *pos, uint32_t len, const char *source);
void consume_multiline_comment(uint32_t *pos, uint32_t *line, uint32_t len, const char *source);
void consume_string_literal(uint32_t *pos, uint32_t *line, uint32_t len, const char *source);
void consume_char_literal(uint32_t *pos, uint32_t len, const char *source);
Token consume_identifier(uint32_t *pos, uint32_t line, uint32_t len, const char *source);
Token consume_compiler_instruction(
        uint32_t *pos, 
        uint32_t line,
        uint32_t len, 
        const char *source, 
        const char const *path
);
Token consume_tt(uint32_t *pos, uint32_t line, TokenType type);
Token consume_o(
        uint32_t *pos, 
        uint32_t line, 
        uint32_t len, 
        const char *source, 
        TokenType type, 
        char nxt, 
        TokenType other
);
ArrayToken tokenize(
        Arena *arena, 
        const char const *path, 
        const char *source, 
        uint32_t len
);
bool is_func(uint32_t pos, ArrayToken *toks);
char* read_file(Arena *arena, const char *path, uint32_t *len);
LineRange consume_define_pragma_and_include(uint32_t *pos, ArrayToken *toks, const char *source);
bool is_compiler_if_intrinsic(TokenType type);
LineRange consume_if(uint32_t *pos, ArrayToken *toks, const char *source);
CharRange consume_struct_or_enum(uint32_t *pos, ArrayToken *toks, const char *source);
CharRange consume_typedef(uint32_t *pos, ArrayToken *toks, const char *source);
CharRange consume_func(uint32_t *pos, ArrayToken *toks, const char *source);
CharRange consume_func_or_global(
        uint32_t *pos, 
        bool is_function, 
        ArrayToken *toks, 
        const char *source
);
FileContent parse_c_file(Arena *arena, Arena *files, const char *path);
void append_string(Arena *arena, StringBuilder *sb, const char *str);
void append_line_range(
        Arena *arena, 
        StringBuilder *sb, 
        LineRange lr, 
        const char *source
);
void append_line_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayLineRange alr,
        const char *source
);
void append_char_range(
        Arena *arena,
        StringBuilder *sb,
        CharRange cr,
        const char *source
);
void append_char_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayCharRange acr,
        const char *source
);
void append_ident(
        Arena *arena, 
        StringBuilder *sb, 
        CharRange cr,
        const char *source
);
void write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

int main(int argc, char **argv) {
    ptrdiff_t file_buffer_size = PATH_COUNT * 1024 * 1024;

    Arena app, files = {0};
    app.beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    app.end = app.beg + file_buffer_size; 

    files.beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    files.end = files.beg + file_buffer_size; 

    ArrayFileContent contents = {0}; 
    ArrayCharPtr file_starts = {0};

    // NOTE: could be multithreaded
    for (uint32_t i = 0; i < PATH_COUNT; i++) {
        printf("Proccessing: `%s`\n", PATHS[i]);
        *push(&file_starts, &app) = files.beg;
        *push(&contents, &app) = parse_c_file(&app, &files, PATHS[i]);
    }

    write_file(&app, UNIT_PATH, file_starts, contents);
    return 0;
}

void *arena_alloc(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, bool zero) {
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    ptrdiff_t available = a->end - a->beg - padding;

    if (available < 0 || count > available / size) {
        assert(false && "TODO: realloc");
    }

    void *p = a->beg + padding;
    a->beg += padding + count * size;

    if (!zero) {
        return p;
    }

    return memset(p, 0, count * size);
}

void grow(void *slice, ptrdiff_t size, Arena *a) {
    struct {
        void     *data;
        ptrdiff_t len;
        ptrdiff_t cap;
    } replica;
    memcpy(&replica, slice, sizeof(replica));

    replica.cap = replica.cap ? replica.cap : 1;
    ptrdiff_t align = 16;
    void *data = arena_alloc(a, 2*size, align, replica.cap, true);
    replica.cap *= 2;
    if (replica.len) {
        memcpy(data, replica.data, size*replica.len);
    }
    replica.data = data;
    memcpy(slice, &replica, sizeof(replica));
}

bool is_ident_start(char character) {
    return character >= 'a' && character <= 'z' || character >= 'A' && character <= 'Z';
}

bool is_ident(char character) {
    return is_ident_start(character) || character >= '0' && character <= '9';
}

bool is_line_comment(uint32_t pos, uint32_t len, const char *source) {
    return pos + 1 < len && source[pos] == '/' && source[pos + 1] == '/';
}

bool is_multiline_comment(uint32_t pos, uint32_t len, const char *source) {
    return pos + 1 < len && source[pos] == '/' && source[pos + 1] == '*';
}

void consume_line_comment(uint32_t *pos, uint32_t len, const char *source) {
    while (*pos < len && source[*pos] != '\n') {
        *pos += 1;
    }
}

void consume_multiline_comment(uint32_t *pos, uint32_t *line, uint32_t len, const char *source) {
    while (*pos < len) {
        *pos += 1;
        if (source[*pos] == '\n') *line += 1;
        else if (source[*pos - 1] == '*' && source[*pos] == '/') break;
    }
    *pos += 1;
}

void consume_string_literal(uint32_t *pos, uint32_t *line, uint32_t len, const char *source) {
    *pos += 1;

    while (*pos < len && source[*pos] != '"') {
        if      (source[*pos] == '\\') *pos += 1;
        else if (source[*pos] == '\n') *line += 1;

        *pos += 1;
    }

    *pos += 1;
}

void consume_char_literal(uint32_t *pos, uint32_t len, const char *source) {
    assert(source[*pos] == '\'' && "ERROR: character literal does not start with -> `'`!");

    *pos += 1;
    if (*pos < len && source[*pos] == '\\') *pos += 1;
    *pos += 2;

    assert(*pos - 1 < len && source[*pos - 1] == '\'' && "ERROR: character literal does not end with -> `'`!");
}

Token consume_identifier(uint32_t *pos, uint32_t line, uint32_t len, const char *source) {
    Token tok = {
        .beg  = *pos,
        .line = line,
        .type = T_IDENT,
    };

    while (*pos < len && is_ident(source[*pos])) {
        *pos += 1;
    }

    uint32_t diff = *pos - tok.beg;

    if          (diff == 6 && strncmp(source + tok.beg, "struct", diff) == 0) {
        tok.type = T_STRUCT;
    } else if   (diff == 4 && strncmp(source + tok.beg, "enum", diff) == 0) {
        tok.type = T_ENUM;
    } else if   (diff == 7 && strncmp(source + tok.beg, "typedef", diff) == 0) {
        tok.type = T_TYPEDEF;
    }

    return tok;
}

Token consume_compiler_instruction(
        uint32_t *pos, 
        uint32_t line,
        uint32_t len, 
        const char *source, 
        const char const *path
) {
    Token tok = {
        .beg = *pos,
        .line = line,
        .type = C_INCLUDE,
    };
    *pos += 1;

    while (*pos < len && is_ident(source[*pos])) {
        *pos += 1;
    }

    uint32_t diff = *pos - tok.beg;

    if          (diff == 8 && strncmp(source + tok.beg, "#include", diff) == 0) {
        tok.type = C_INCLUDE;
    } else if   (diff == 7 && strncmp(source + tok.beg, "#define", diff) == 0) {
        tok.type = C_DEFINE;
    } else if   (diff == 7 && strncmp(source + tok.beg, "#pragma", diff) == 0) {
        tok.type = C_PRAGMA;
    } else if   (diff == 6 && strncmp(source + tok.beg, "#undef", diff) == 0) {
        tok.type = C_UNDEF;
    } else if   (diff == 3 && strncmp(source + tok.beg, "#if", diff) == 0) {
        tok.type = C_IF;
    } else if   (diff == 6 && strncmp(source + tok.beg, "#ifdef", diff) == 0) {
        tok.type = C_IFDEF;
    } else if   (diff == 7 && strncmp(source + tok.beg, "#ifndef", diff) == 0) {
        tok.type = C_IFNDEF;
    } else if   (diff == 5 && strncmp(source + tok.beg, "#else", diff) == 0) {
        tok.type = C_ELSE;
    } else if   (diff == 5 && strncmp(source + tok.beg, "#elif", diff) == 0) {
        tok.type = C_ELIF;
    } else if   (diff == 6 && strncmp(source + tok.beg, "#endif", diff) == 0) {
        tok.type = C_ENDIF;
    } else if   (diff == 6 && strncmp(source + tok.beg, "#error", diff) == 0) {
        tok.type = C_ERROR;
    } else {
        printf("ERROR: either unknown compiler intrinsic or invalid c file -> `%s`, line=%d\n", path, line);
        assert(false);
    }

    return tok;
}

Token consume_tt(uint32_t *pos, uint32_t line, TokenType type) {
    Token tok = {
        .beg = *pos,
        .line = line,
        .type = type,
    };
    *pos += 1;

    return tok;
}

Token consume_o(
        uint32_t *pos, 
        uint32_t line, 
        uint32_t len, 
        const char *source, 
        TokenType type, 
        char nxt, 
        TokenType other
) {
    Token tok = {
        .beg = *pos,
        .line = line,
        .type = type,
    };

    if (*pos + 1 < len && source[*pos + 1] == nxt) {
        tok.type = other;
        *pos += 2;
    } else {
        *pos += 1;
    }

    return tok;
}

ArrayToken tokenize(
        Arena *arena, 
        const char const *path, 
        const char *source, 
        uint32_t len
) {
    ArrayToken tokens = {0};

    uint32_t pos = 0;
    uint32_t line = 1;

    while (pos < len) {
        if          (is_ident_start(source[pos])) {
            *push(&tokens, arena) = consume_identifier(&pos, line, len, source);
        } else if   (source[pos] == '#') {
            *push(&tokens, arena) = consume_compiler_instruction(&pos, line, len, source, path);
        } else if   (is_line_comment(pos, len, source)) {
            consume_line_comment(&pos, len, source);
        } else if   (is_multiline_comment(pos, len, source)) {
            consume_multiline_comment(&pos, &line, len, source);
        } else if   (source[pos] == '\'') {
            consume_char_literal(&pos, len, source);
        } else if   (source[pos] == '"') {
            consume_string_literal(&pos, &line, len, source);
        } else if   (source[pos] == '(') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_L_PAREN);
        } else if   (source[pos] == ')') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_R_PAREN);
        } else if   (source[pos] == '{') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_L_BRACE);
        } else if   (source[pos] == '}') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_R_BRACE);
        } else if   (source[pos] == '[') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_L_BRACKET);
        } else if   (source[pos] == ']') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_R_BRACKET);
        } else if   (source[pos] == '\\') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_BACKSLASH);
        } else if   (source[pos] == ',') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_COMMA);
        } else if   (source[pos] == ';') {
            *push(&tokens, arena) = consume_tt(&pos, line, TT_SEMICOLON);
        } else if   (source[pos] == '=') {
            *push(&tokens, arena) = consume_o(&pos, line, len, source, O_EQ, '=', R_IGNORE);
        } else if   (source[pos] == '\n') {
            pos += 1;
            line += 1;
        } else { pos += 1; }
    }

    *push(&tokens, arena) = (Token){.beg = len, .line = line, .type = R_EOF};
    return tokens;
}

bool is_func(uint32_t pos, ArrayToken *toks) {
    while (pos < toks->len && !(toks->data[pos].type == TT_L_BRACE || toks->data[pos].type == O_EQ)) {
        pos += 1;
    }

    return pos < toks->len && toks->data[pos].type == TT_L_BRACE;
}

char* read_file(Arena *arena, const char *path, uint32_t *len) {
    FILE *f = fopen(path, "r");
    if (f == NULL) assert(false && "error: file not found");

    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    rewind(f);

    char *buf = alloc(arena, char, *len, false);
    fread(buf, sizeof(char), *len, f);

    fclose(f);
    return buf;
}

LineRange consume_define_pragma_and_include(uint32_t *pos, ArrayToken *toks, const char *source) {
    uint32_t iden = *pos;
    uint32_t line = toks->data[iden].line;

    while (toks->data[*pos].type != R_EOF && toks->data[*pos].line == line) {
        if (toks->data[*pos].type == TT_BACKSLASH) {
            line += 1;
        }

        *pos += 1;
    }

    return (LineRange){
        .start = toks->data[iden],
        .end_line = line,
    };
}

bool is_compiler_if_intrinsic(TokenType type) {
    return type == C_IF || type == C_IFDEF || type == C_IFNDEF;
}

LineRange consume_if(uint32_t *pos, ArrayToken *toks, const char *source) {
    Token start = toks->data[*pos];
    uint16_t ifs = 1;
    *pos += 1;

    while (toks->data[*pos].type != R_EOF && ifs > 0) {
        if      (is_compiler_if_intrinsic(toks->data[*pos].type)) ifs += 1;
        else if (toks->data[*pos].type == C_ENDIF) ifs -= 1;

        *pos += 1;
    }

    return (LineRange){
        .start = start,
        .end_line = toks->data[*pos - 1].line,
    };
}

CharRange consume_struct_or_enum(uint32_t *pos, ArrayToken *toks, const char *source) {
    *pos += 1;
    assert(toks->data[*pos].type == T_IDENT && "Error: expected identifier after struct or enum");

    Token iden = toks->data[*pos];
    uint16_t braces = 1;
    *pos += 2;

    while (toks->data[*pos].type != R_EOF && braces > 0) {
        if      (toks->data[*pos].type == TT_L_BRACE) braces += 1;
        else if (toks->data[*pos].type == TT_R_BRACE) braces -= 1;

        *pos += 1;
    }

    Token end_char = toks->data[*pos];

    while (toks->data[*pos].type != R_EOF && toks->data[*pos].type != TT_SEMICOLON) {
        *pos += 1;
    }
    if (toks->data[*pos].type != R_EOF) *pos += 1;

    return (CharRange){
        .start = iden,
        .end_char = end_char,
    };
}

CharRange consume_typedef(uint32_t *pos, ArrayToken *toks, const char *source) {
    Token start = toks->data[*pos];

    *pos += 1;
    TokenType type = toks->data[*pos].type;

    if (type == T_STRUCT || type == T_ENUM) {
        return (CharRange){.start = {0}, .end_char = {0}};
    }

    assert(type == T_IDENT && "Error: expected identifier after typedef");

    while (toks->data[*pos].type != R_EOF && toks->data[*pos].type != TT_SEMICOLON) {
        *pos += 1;
    }
    if (toks->data[*pos].type != R_EOF) *pos += 1;

    return (CharRange){
        .start = start,
        .end_char = toks->data[*pos],
    };
}

CharRange consume_func(uint32_t *pos, ArrayToken *toks, const char *source) {
    Token start = toks->data[*pos];
    uint16_t braces = 1;

    while (*pos < toks->len && toks->data[*pos].type != TT_L_BRACE) {
        *pos += 1;
    }
    *pos += 1;

    while (toks->data[*pos].type != R_EOF && braces > 0) {
        if      (toks->data[*pos].type == TT_L_BRACE) braces += 1;
        else if (toks->data[*pos].type == TT_R_BRACE) braces -= 1;

        *pos += 1;
    }

    return (CharRange){
        .start = start,
        .end_char = toks->data[*pos - 1],
    };
}

CharRange consume_func_or_global(
        uint32_t *pos, 
        bool is_function, 
        ArrayToken *toks, 
        const char *source
) {
    if (is_function) {
        return consume_func(pos, toks, source);
    }

    Token start = toks->data[*pos];

    while (toks->data[*pos].type != R_EOF && toks->data[*pos].type != TT_SEMICOLON) {
        *pos += 1;
    }
    if (toks->data[*pos].type != R_EOF) *pos += 1;

    return (CharRange){
        .start = start,
        .end_char = toks->data[*pos - 1],
    };
}

FileContent parse_c_file(Arena *arena, Arena *files, const char *path) {
    uint32_t len    = 0;
    uint32_t pos    = 0;
    const char *source = read_file(files, path, &len);

    ArrayToken toks = tokenize(arena, path, source, len);
    FileContent content = {0};

    while (toks.data[pos].type != R_EOF) {
        TokenType type = toks.data[pos].type;

        if          (type == C_DEFINE) {
            *push(&content.defines, arena) = consume_define_pragma_and_include(&pos, &toks, source);
        } else if   (type == C_PRAGMA) {
            *push(&content.pragmas, arena) = consume_define_pragma_and_include(&pos, &toks, source);
        } else if   (type == C_INCLUDE || type == C_ERROR) {
            *push(&content.includes, arena) = consume_define_pragma_and_include(&pos, &toks, source);
        } else if   (is_compiler_if_intrinsic(type)) {
            *push(&content.compiler_ifs, arena) = consume_if(&pos, &toks, source);
        } else if   (type == T_TYPEDEF) {
            CharRange c_range = consume_typedef(&pos, &toks, source);
            if (c_range.start.beg == 0 && c_range.end_char.beg == 0) continue;

            *push(&content.typedefs, arena) = c_range;
        } else if   (type == T_ENUM) {
            *push(&content.enums, arena) = consume_struct_or_enum(&pos, &toks, source);
        } else if   (type == T_STRUCT) {
            *push(&content.structs, arena) = consume_struct_or_enum(&pos, &toks, source);
        } else if   (type == T_IDENT) {
            bool is_function = is_func(pos, &toks);
            CharRange c_range = consume_func_or_global(&pos, is_function, &toks, source);

            if (is_function) {
                *push(&content.functions, arena) = c_range;
            } else {
                *push(&content.globals, arena) = c_range;
            }
        } else {
            printf("Error: encountered unreachable state - token_type=%d\n", toks.data[pos].type);
            exit(1);
        }
    }

    return content;
}

void append_string(Arena *arena, StringBuilder *sb, const char *str) {
    for (uint32_t i = 0; i < strlen(str); i++) {
        *push(sb, arena) = str[i];
    }
}

void append_line_range(
        Arena *arena, 
        StringBuilder *sb, 
        LineRange lr, 
        const char *source
) {
    uint32_t pos    = lr.start.beg;
    uint32_t line   = lr.start.line;
    uint32_t end_line = lr.end_line;

    while (line <= end_line) {
        if (source[pos] == '\n') line++;
        *push(sb, arena) = source[pos];
        pos += 1;
    }
}

void append_line_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayLineRange alr,
        const char *source
) {
    for (uint32_t i = 0; i < alr.len; i++) {
        append_line_range(arena, sb, alr.data[i], source);
    }
}

void append_char_range(
        Arena *arena,
        StringBuilder *sb,
        CharRange cr,
        const char *source
) {
    uint32_t pos = cr.start.beg;
    uint32_t end = cr.end_char.beg;

    while (pos < end) {
        *push(sb, arena) = source[pos];
        pos += 1;
    }
}

void append_char_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayCharRange acr,
        const char *source
) {
    for (uint32_t i = 0; i < acr.len; i++) {
        append_char_range(arena, sb, acr.data[i], source);
    }
}

void append_ident(
        Arena *arena, 
        StringBuilder *sb, 
        CharRange cr,
        const char *source
) {
    uint32_t pos = cr.start.beg;

    while (is_ident(source[pos])) {
        *push(sb, arena) = source[pos];
        pos += 1;
    }
}

void write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents) {
    FILE *unit_file = fopen(create_path, "w");
    if (unit_file == NULL) {
        printf("Error: unable to create `%create_path`\n", create_path);
        exit(1);
    }

    StringBuilder sb = {0};

    // APPENDING INCLUDES
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, contents.data[i].includes, file_starts.data[i]);
    }
    *push(&sb, arena) = '\n';

    // APPENDING DEFINES
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, contents.data[i].defines, file_starts.data[i]);
    }
    *push(&sb, arena) = '\n';

    // APPENDING PRAGMAS
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, contents.data[i].pragmas, file_starts.data[i]);
    }
    *push(&sb, arena) = '\n';
 
    // APPENDING ENUMS
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange enums = contents.data[i].enums;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < enums.len; j++) {
            append_string(arena, &sb, "typedef enum ");
            append_char_range(arena, &sb, enums.data[j], source);
            append_ident(arena, &sb, enums.data[j], source);
            append_string(arena, &sb, ";\n\n");
        }
    }
    *push(&sb, arena) = '\n';

    // APPENDING STRUCTS
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            append_string(arena, &sb, "typedef struct ");
            append_ident(arena, &sb, structs.data[j], source);
            *push(&sb, arena) = ' ';
            append_ident(arena, &sb, structs.data[j], source);
            append_string(arena, &sb, ";\n");
        }
    }
    *push(&sb, arena) = '\n';

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            append_string(arena, &sb, "typedef struct ");
            append_char_range(arena, &sb, structs.data[j], source);
            *push(&sb, arena) = ' ';
            append_ident(arena, &sb, structs.data[j], source);
            append_string(arena, &sb, ";\n\n");
        }
    }

    // APPENDING compiler ifs
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, contents.data[i].compiler_ifs, file_starts.data[i]);
    }
    *push(&sb, arena) = '\n';

    // APPENDING globals
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange globals = contents.data[i].globals;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < globals.len; j++) {
            append_char_range(arena, &sb, globals.data[j], source);
            append_string(arena, &sb, ";\n");
        }
    }
    *push(&sb, arena) = '\n'; 

    // APPEND functions
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;

            while (source[pos] != '{') {
                *push(&sb, arena) = source[pos];
                pos += 1;
            }
            sb.data[sb.len - 1] = ';';
            *push(&sb, arena) = '\n';
        }
    }
    *push(&sb, arena) = '\n';

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;
            uint32_t end = funcs.data[j].end_char.beg;

            while (pos <= end) {
                *push(&sb, arena) = source[pos];
                pos += 1;
            }
            append_string(arena, &sb, "\n\n");
        }
    }

    fwrite(sb.data, sizeof(sb.data[0]), sb.len, unit_file);
    fclose(unit_file);
}

