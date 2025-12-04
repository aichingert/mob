#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <sys/mman.h>

#define UNIT_PATH       "app.c"
#define UNIT_PATH_LEN   sizeof(UNIT_PATH) / sizeof(UNIT_PATH[0]) - 1

static const char *FLAGS[] = {
    "-std=c23",
    "-ffreestanding",
    "-Wextra",
    "-Wall",
    // TODO: remove this one
    "-Wno-type-limits",
    //"-Wfloat-equal",
    //"-Wundef",
    //"-Wshadow",
    //"-Wpointer-arith",
    //"-Wcast-align",
    //"-Wstrict-prototypes",
    //"-Wstrict-overflow=5",
    //"-Wwrite-strings",
    //"-Waggregate-return"
};
static const uint32_t FLAG_COUNT = sizeof(FLAGS) / sizeof(FLAGS[0]);
static const char *PATHS[] = {
    "std/mem.c",
    "std/math.c",
    "std/file.c",
    "std/types.c",
    "std/arena.c",
    "std/string.c",
    "std/collection.c",
    "std/unix_os.c",
    "std/unix_sys.c",
    "std/unix_socket.c",
    "std/unix_window.c",
    "mob.c",
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

enum TokenType {
    T_ENUM          = 0,
    T_STRUCT        = 1,
    T_TYPEDEF       = 2,
    T_IDENT         = 3,

    C_IF            = 20,
    C_IFDEF         = 21,
    C_IFNDEF        = 22,
    C_ELSE          = 23,
    C_ELIF          = 24,
    C_ENDIF         = 25,
    C_UNDEF         = 26,
    C_ERROR         = 27,
    C_PRAGMA        = 28,
    C_DEFINE        = 29,
    C_INCLUDE       = 30,
    C_MACRO_CONCAT  = 31,

    O_EQ            = 40,

    TT_L_BRACE      = 60,
    TT_R_BRACE      = 61,
    TT_L_PAREN      = 62,
    TT_R_PAREN      = 63,
    TT_L_BRACKET    = 64,
    TT_R_BRACKET    = 65,
    TT_COMMA        = 66,
    TT_SEMICOLON    = 67,
    TT_BACKSLASH    = 68,

    R_EOF           = 80,
    R_IGNORE        = 81,
};

typedef struct Token {
    uint32_t    beg;
    uint32_t    line;
    enum TokenType   type;
} Token;

typedef struct ArrayToken {
    Token *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayToken;

bool is_ident(char character);
ArrayToken tokenize(Arena *arena, const char *path, const char *source, uint32_t len);

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

char *read_file(Arena *arena, FILE *file, uint32_t *len);
FileContent parse_c_file(Arena *arena, Arena *scratch, const char *path);

// NOTE: maybe improve this if it leads to bottlenecks
typedef struct StringBuilder {
    char *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} StringBuilder;

typedef struct CopyPosition {
    uint32_t file;
    uint32_t file_line;
    uint32_t start_line;
    uint32_t end_line;
} CopyPosition;

typedef struct ArrayCopyPosition {
    CopyPosition *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayCopyPosition;

typedef struct FileSections {
    uint32_t include_end;
    uint32_t include_arr;
    uint32_t define_end;
    uint32_t define_arr;
    uint32_t pragma_end;
    uint32_t pragma_arr;
    uint32_t typedef_end;
    uint32_t typedef_arr;
    uint32_t enum_end;
    uint32_t enum_arr;
    uint32_t struct_declare_end;
    uint32_t struct_define_end;
    uint32_t struct_arr;
    uint32_t compiler_if_end;
    uint32_t compiler_if_arr;
    uint32_t function_header_end;
    uint32_t global_variable_end;
    uint32_t global_variable_arr;
    uint32_t function_define_end;
    uint32_t function_arr;

    ArrayCopyPosition positions;
} FileSections;

uint32_t append_string(Arena *arena, StringBuilder *sb, const char *str);
void append_number(Arena *arena, StringBuilder *sb, int64_t number);
FileSections write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

void mob_compile(
        Arena *arena, 
        const char *unit_path, 
        uint32_t unit_path_len, 
        const char **flags,
        uint32_t flags_len,
        const char **paths, 
        FileSections *fs
);

void allocate_arenas(Arena *app, Arena *files, ptrdiff_t file_buffer_size);
int main(void);
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
        const char *path
);
Token consume_tt(uint32_t *pos, uint32_t line, enum TokenType type);
Token consume_o(
        uint32_t *pos, 
        uint32_t line, 
        uint32_t len, 
        const char *source, 
        enum TokenType type, 
        char nxt, 
        enum TokenType other
);
ArrayToken tokenize(
        Arena *arena, 
        const char *path, 
        const char *source, 
        uint32_t len
);
bool is_func(uint32_t pos, ArrayToken *toks);
char* read_file(Arena *arena, FILE *file, uint32_t *len);
LineRange consume_define_pragma_and_include(uint32_t *pos, ArrayToken *toks);
bool is_compiler_if_intrinsic(enum TokenType type);
LineRange consume_if(uint32_t *pos, ArrayToken *toks);
CharRange consume_struct_or_enum(uint32_t *pos, ArrayToken *toks);
CharRange consume_typedef(uint32_t *pos, ArrayToken *toks);
CharRange consume_func(uint32_t *pos, ArrayToken *toks);
CharRange consume_func_or_global(
        uint32_t *pos, 
        bool is_function, 
        ArrayToken *toks
);
FileContent parse_c_file(Arena *arena, Arena *files, const char *path);
void append_newline(Arena *arena, StringBuilder *sb, uint32_t *line_nr);
uint32_t append_string(Arena *arena, StringBuilder *sb, const char *str);
void append_number(Arena *arena, StringBuilder *sb, int64_t number);
uint32_t append_line_range(
        Arena *arena, 
        StringBuilder *sb, 
        LineRange lr, 
        const char *source
);
void append_line_ranges(
        Arena *arena,
        StringBuilder *sb,
        uint32_t *line_nr,
        ArrayLineRange alr,
        const char *source,
        uint32_t file,
        FileSections *fs
);
uint32_t append_char_range(
        Arena *arena,
        StringBuilder *sb,
        CharRange cr,
        const char *source
);
uint32_t append_char_ranges(
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
FileSections write_file(
        Arena *arena, 
        const char *create_path, 
        ArrayCharPtr file_starts, 
        ArrayFileContent contents
);
bool is_numeric(char c);
uint32_t reverse_number(uint32_t number);
uint32_t string_len_of_number(uint32_t number);
bool is_duplicated_error(uint32_t line, FileSections *fs);
bool is_include_error(uint32_t line, FileSections *fs);
bool is_define_error(uint32_t line, FileSections *fs);
bool is_pragma_error(uint32_t line, FileSections *fs);
bool is_typedef_error(uint32_t line, FileSections *fs);
bool is_enum_error(uint32_t line, FileSections *fs);
bool is_struct_error(uint32_t line, FileSections *fs);
bool is_comp_if_error(uint32_t line, FileSections *fs);
bool is_global_var_error(uint32_t line, FileSections *fs);
bool is_function_error(uint32_t line, FileSections *fs);
void error_skip_to_next(uint32_t *pos, StringBuilder sb, const char *path, uint32_t path_len);
uint32_t error_skip_if_not_location(uint32_t *pos, StringBuilder sb, uint32_t path_len);
LineRange error_extract_position(uint32_t *pos, StringBuilder sb, uint32_t path_len);
void error_append_remaining_till_next(
        Arena *arena,
        uint32_t *pos, 
        StringBuilder sb, 
        StringBuilder *new_error,
        const char *unit_path, 
        uint32_t unit_path_len,
        uint32_t replace_line,
        uint32_t replace_with
);
CopyPosition error_search_range(uint32_t line, uint32_t start, uint32_t end, FileSections *fs);
CopyPosition error_get_correct_location(uint32_t line, FileSections *fs);
uint32_t error_append_original_location(
        Arena *arena,
        StringBuilder *sb,
        StringBuilder *err_msg, 
        const char **paths, 
        uint32_t unit_path_len,
        uint32_t line, 
        uint32_t position,
        uint32_t start,
        uint32_t end,
        FileSections *fs
);
void concat_compile_command(
        const char *compiler_path, 
        const char **flags,
        uint32_t flags_len,
        const char *c_source_path,
        uint32_t c_source_path_len,
        char *cmd
);
void mob_compile(
        Arena *arena, 
        const char *unit_path, 
        uint32_t unit_path_len, 
        const char **flags,
        uint32_t flags_len,
        const char **paths,
        FileSections *fs
);
void allocate_arenas(Arena *app, Arena *files, ptrdiff_t file_buffer_size) {
#if _win32
	app->beg = VirtualAlloc(NULL, file_buffer_size, MEM_COMMIT, PAGE_READWRITE);
    app->end = app->beg + file_buffer_size; 

	files->beg = VirtualAlloc(NULL, file_buffer_size, MEM_COMMIT, PAGE_READWRITE);
    files->end = files->beg + file_buffer_size; 
#elif __unix__
	app->beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    app->end = app->beg + file_buffer_size; 

    files->beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    files->end = files->beg + file_buffer_size; 
#endif
}

int main(void) {
    ptrdiff_t file_buffer_size = 10 * PATH_COUNT * 1024 * 1024;

    Arena app   = {0};
    Arena files = {0};
    allocate_arenas(&app, &files, file_buffer_size);

    ArrayFileContent contents = {0}; 
    ArrayCharPtr file_starts = {0};

    // NOTE: could be multithreaded
    for (uint32_t i = 0; i < PATH_COUNT; i++) {
        printf("[INFO]: proccessing -> `%s`\n", PATHS[i]);
        *push(&file_starts, &app) = files.beg;
        *push(&contents, &app) = parse_c_file(&app, &files, PATHS[i]);
    }

    FileSections fs = write_file(&app, UNIT_PATH, file_starts, contents);
    mob_compile(&app, UNIT_PATH, UNIT_PATH_LEN, FLAGS, FLAG_COUNT, PATHS, &fs);

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
    return (character >= 'a' && character <= 'z') 
        || (character >= 'A' && character <= 'Z');
}

bool is_ident(char character) {
    return is_ident_start(character) 
        || (character >= '0' && character <= '9')
        || (character == '_');
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
        const char *path
) {
    Token tok = {
        .beg = *pos,
        .line = line,
        .type = C_INCLUDE,
    };
    *pos += 1;

    while (*pos < len && (is_ident(source[*pos]) || source[*pos] == '#')) {
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
    } else if   (diff == 2 && strncmp(source + tok.beg, "##", diff) == 0) {
        tok.type = C_MACRO_CONCAT;
    } else {
        printf("ERROR: either unknown compiler intrinsic or invalid c file -> `%s`, line=%d\n", path, line);
        assert(false);
    }

    return tok;
}

Token consume_tt(uint32_t *pos, uint32_t line, enum TokenType type) {
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
        enum TokenType type, 
        char nxt, 
        enum TokenType other
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
        const char *path, 
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
    while (pos < toks->len && !(toks->data[pos].type == TT_L_PAREN || toks->data[pos].type == O_EQ)) {
        pos += 1;
    }

    if (pos < toks->len && toks->data[pos].type == TT_L_PAREN) {
        while (pos < toks->len && (toks->data[pos].type != TT_L_BRACE 
                                && toks->data[pos].type != O_EQ
                                && toks->data[pos].type != TT_SEMICOLON)) {
            pos += 1;
        }

        return pos < toks->len && toks->data[pos].type == TT_L_BRACE;
    }

    return pos < toks->len && toks->data[pos].type == TT_L_BRACE;
}

char* read_file(Arena *arena, FILE *file, uint32_t *len) {
    if (file == NULL) assert(false && "Error(read_file): file is null");

    fseek(file, 0, SEEK_END);
    *len = ftell(file);
    rewind(file);

    char *buf = (char*)alloc(arena, uint8_t, *len, false);
    fread(buf, sizeof(char), *len, file);

    return buf;
}

LineRange consume_define_pragma_and_include(uint32_t *pos, ArrayToken *toks) {
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

bool is_compiler_if_intrinsic(enum TokenType type) {
    return type == C_IF || type == C_IFDEF || type == C_IFNDEF;
}

LineRange consume_if(uint32_t *pos, ArrayToken *toks) {
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

CharRange consume_struct_or_enum(uint32_t *pos, ArrayToken *toks) {
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

CharRange consume_typedef(uint32_t *pos, ArrayToken *toks) {
    Token start = toks->data[*pos];

    *pos += 1;
    enum TokenType type = toks->data[*pos].type;

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

CharRange consume_func(uint32_t *pos, ArrayToken *toks) {
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
        ArrayToken *toks
) {
    if (is_function) {
        return consume_func(pos, toks);
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
    FILE *file = fopen(path, "r");
    const char *source = read_file(files, file, &len);
    fclose(file);

    ArrayToken toks = tokenize(arena, path, source, len);
    FileContent content = {0};

    while (toks.data[pos].type != R_EOF) {
        enum TokenType type = toks.data[pos].type;

        if          (type == C_DEFINE) {
            *push(&content.defines, arena) = consume_define_pragma_and_include(&pos, &toks);
        } else if   (type == C_PRAGMA) {
            *push(&content.pragmas, arena) = consume_define_pragma_and_include(&pos, &toks);
        } else if   (type == C_INCLUDE || type == C_ERROR) {
            *push(&content.includes, arena) = consume_define_pragma_and_include(&pos, &toks);
        } else if   (is_compiler_if_intrinsic(type)) {
            *push(&content.compiler_ifs, arena) = consume_if(&pos, &toks);
        } else if   (type == T_TYPEDEF) {
            CharRange c_range = consume_typedef(&pos, &toks);
            if (c_range.start.beg == 0 && c_range.end_char.beg == 0) continue;

            *push(&content.typedefs, arena) = c_range;
        } else if   (type == T_ENUM) {
            *push(&content.enums, arena) = consume_struct_or_enum(&pos, &toks);
        } else if   (type == T_STRUCT) {
            *push(&content.structs, arena) = consume_struct_or_enum(&pos, &toks);
        } else if   (type == T_IDENT) {
            bool is_function = is_func(pos, &toks);
            CharRange c_range = consume_func_or_global(&pos, is_function, &toks);

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

void append_newline(Arena *arena, StringBuilder *sb, uint32_t *line_nr) {
    *line_nr += 1;
    *push(sb, arena) = '\n';
}

uint32_t append_string(Arena *arena, StringBuilder *sb, const char *str) {
    uint32_t lines = 0;

    for (uint32_t i = 0; i < strlen(str); i++) {
        *push(sb, arena) = str[i];
        if (str[i] == '\n') lines += 1;
    }

    return lines;
}

void append_number(Arena *arena, StringBuilder *sb, int64_t number) {
    if (number < 0) {
        *push(sb, arena) = '-';
        number *= -1;
    }

    int64_t copy = number;
    uint32_t len = 0;

    while (copy > 0) {
        len += 1;
        copy /= 10;
        *push(sb, arena) = '0';
    }

    if (len == 0) {
        *push(sb, arena) = '0';
        return;
    }

    for (uint32_t i = 1; i <= len; i++) {
        uint8_t d = number % 10;
        number /= 10;
        sb->data[sb->len - i] = d + '0';
    } 
}

uint32_t append_line_range(
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

    return line - lr.start.line;
}

void append_line_ranges(
        Arena *arena,
        StringBuilder *sb,
        uint32_t *line_nr,
        ArrayLineRange alr,
        const char *source,
        uint32_t file,
        FileSections *fs
) {
    for (uint32_t i = 0; i < alr.len; i++) {
        uint32_t line_start = *line_nr;
        *line_nr += append_line_range(arena, sb, alr.data[i], source);

        *push(&fs->positions, arena) = (CopyPosition){
            .file = file,
            .file_line = alr.data[i].start.line,
            .start_line = line_start,
            .end_line = *line_nr,
        };
    }
}

uint32_t append_char_range(
        Arena *arena,
        StringBuilder *sb,
        CharRange cr,
        const char *source
) {
    uint32_t pos = cr.start.beg;
    uint32_t end = cr.end_char.beg;
    uint32_t line = 0;

    while (pos < end) {
        if (source[pos] == '\n') line += 1;
        *push(sb, arena) = source[pos];
        pos += 1;
    }

    return line;
}

uint32_t append_char_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayCharRange acr,
        const char *source
) {
    uint32_t lines = 0;

    for (uint32_t i = 0; i < acr.len; i++) {
        lines += append_char_range(arena, sb, acr.data[i], source);
    }

    return lines;
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

FileSections write_file(
        Arena *arena, 
        const char *create_path, 
        ArrayCharPtr file_starts, 
        ArrayFileContent contents
) {
    FILE *process = fopen(create_path, "w");
    if (process == NULL) {
        printf("Error: unable to create `%s`\n", create_path);
        exit(1);
    }

    uint32_t line_nr = 0;
    FileSections fs = {0};
    StringBuilder sb = {0};

    // APPENDING includes 
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].includes, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.include_end = line_nr;
    fs.include_arr = fs.positions.len;

    // APPENDING defines
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].defines, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.define_end = line_nr;
    fs.define_arr = fs.positions.len;

    // APPENDING pragmas 
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].pragmas, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.pragma_end = line_nr;
    fs.pragma_arr = fs.positions.len;

    // APPENDING typedefs
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange typedefs = contents.data[i].typedefs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < typedefs.len; j++) {
            CopyPosition cp = {
                .file = i,
                .file_line = typedefs.data[j].start.line,
                .start_line = line_nr,
            };

            line_nr += append_char_range(arena, &sb, typedefs.data[j], source);
            cp.end_line = line_nr;
            *push(&fs.positions, arena) = cp;
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.typedef_end = line_nr;
    fs.typedef_arr = fs.positions.len;
 
    // APPENDING enums
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange enums = contents.data[i].enums;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < enums.len; j++) {
            CopyPosition cp = {
                .file = i,
                .file_line = enums.data[j].start.line,
                .start_line = line_nr,
            };

            append_string(arena, &sb, "typedef enum ");
            line_nr += append_char_range(arena, &sb, enums.data[j], source);
            append_ident(arena, &sb, enums.data[j], source);
            line_nr += append_string(arena, &sb, ";\n\n");

            cp.end_line = line_nr;
            *push(&fs.positions, arena) = cp;
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.enum_end = line_nr;
    fs.enum_arr = fs.positions.len;

    // APPENDING typedefs for structs
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            append_string(arena, &sb, "typedef struct ");
            append_ident(arena, &sb, structs.data[j], source);
            *push(&sb, arena) = ' ';
            append_ident(arena, &sb, structs.data[j], source);
            line_nr += append_string(arena, &sb, ";\n"); 
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.struct_declare_end = line_nr;

    // APPENDING structs
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            CopyPosition cp = {
                .file = i,
                .file_line = structs.data[j].start.line,
                .start_line = line_nr,
            };

            append_string(arena, &sb, "typedef struct ");
            line_nr += append_char_range(arena, &sb, structs.data[j], source);
            *push(&sb, arena) = ' ';
            append_ident(arena, &sb, structs.data[j], source);
            line_nr += append_string(arena, &sb, ";\n\n");

            cp.end_line = line_nr;
            *push(&fs.positions, arena) = cp;
        }
    }
    fs.struct_define_end = line_nr;
    fs.struct_arr = fs.positions.len;

    // APPENDING compiler ifs
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].compiler_ifs, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.compiler_if_end = line_nr;
    fs.compiler_if_arr = fs.positions.len;

    // APPENDING function headers
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;

            while (source[pos] != '{') {
                if (source[pos] == '\n') line_nr += 1;
                *push(&sb, arena) = source[pos];
                pos += 1;
            }
            sb.data[sb.len - 1] = ';';
            append_newline(arena, &sb, &line_nr);
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.function_header_end = line_nr;


    // APPENDING globals
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange globals = contents.data[i].globals;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < globals.len; j++) {
            CopyPosition cp = {
                .file = i,
                .file_line = globals.data[j].start.line,
                .start_line = line_nr,
            };

            line_nr += append_char_range(arena, &sb, globals.data[j], source);
            line_nr += append_string(arena, &sb, ";\n");

            cp.end_line = line_nr;
            *push(&fs.positions, arena) = cp;
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.global_variable_end = line_nr;
    fs.global_variable_arr = fs.positions.len;

    // APPEND functions
    
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            CopyPosition cp = {
                .file = i,
                .file_line = funcs.data[j].start.line,
                .start_line = line_nr,
            };
            uint32_t pos = funcs.data[j].start.beg;
            uint32_t end = funcs.data[j].end_char.beg;

            while (pos <= end) {
                if (source[pos] == '\n') line_nr += 1;
                *push(&sb, arena) = source[pos];
                pos += 1;
            }
            line_nr += append_string(arena, &sb, "\n\n");

            cp.end_line = line_nr;
            *push(&fs.positions, arena) = cp;
        }
    }
    fs.function_define_end = line_nr;
    fs.function_arr = fs.positions.len;

    fwrite(sb.data, sizeof(sb.data[0]), sb.len, process);
    fclose(process);

    return fs;
}

bool is_numeric(char c) {
    return (c >= '0') && (c <= '9');
}

uint32_t reverse_number(uint32_t number) {
    uint32_t reverse = 0;

    while (number > 0) {
        reverse *= 10;
        reverse += number % 10;
        number /= 10;
    }

    return reverse;
}

uint32_t string_len_of_number(uint32_t number) {
    uint32_t len = 0;

    while (number > 0) {
        number /= 10;
        len += 1;
    }

    return len == 0 ? len + 1 : len;
}

bool is_duplicated_error(uint32_t line, FileSections *fs) {
    return (line >= fs->enum_end        && line < fs->struct_declare_end)
        || (line >= fs->compiler_if_end && line < fs->function_header_end);
}

bool is_include_error(uint32_t line, FileSections *fs) {
    return line < fs->include_end && fs->include_arr > 0;
}

bool is_define_error(uint32_t line, FileSections *fs) {
    return line >= fs->include_end && line < fs->define_end && fs->define_arr - fs->include_arr > 0;
}

bool is_pragma_error(uint32_t line, FileSections *fs) {
    return line >= fs->define_end && line < fs->pragma_end && fs->pragma_arr - fs->define_arr > 0;
}

bool is_typedef_error(uint32_t line, FileSections *fs) {
    return line >= fs->pragma_end && line < fs->typedef_end && fs->typedef_arr - fs->pragma_arr > 0;
}

bool is_enum_error(uint32_t line, FileSections *fs) {
    return line >= fs->typedef_end && line < fs->enum_end && fs->enum_arr - fs->typedef_arr > 0;
}

bool is_struct_error(uint32_t line, FileSections *fs) {
    return line >= fs->struct_declare_end && line < fs->struct_define_end && fs->struct_arr - fs->enum_arr > 0;
}

bool is_comp_if_error(uint32_t line, FileSections *fs) {
    return line >= fs->struct_define_end && line < fs->compiler_if_end && fs->compiler_if_arr - fs->struct_arr > 0;
}

bool is_global_var_error(uint32_t line, FileSections *fs) {
    return line >= fs->function_header_end 
        && line < fs->global_variable_end 
        && fs->global_variable_arr - fs->compiler_if_arr > 0;
}

bool is_function_error(uint32_t line, FileSections *fs) {
    return line >= fs->global_variable_end 
        && line < fs->function_define_end
        && fs->function_arr - fs->global_variable_arr > 0;
}

void error_skip_to_next(uint32_t *pos, StringBuilder sb, const char *path, uint32_t path_len) {
    while (*pos + path_len < sb.len) {
        bool is_next_error = true;

        // TODO: better skipping
        for (uint32_t i = 0; i < path_len; i++) {
            if (sb.data[*pos + i] != path[i]) {
                is_next_error = false;
                break;
            }
        }

        if (is_next_error) {
            return;
        }

        *pos += 1;
    }

    *pos = sb.len;
}

uint32_t error_skip_if_not_location(uint32_t *pos, StringBuilder sb, uint32_t path_len) {
    *pos += path_len + 1;

    if (*pos < sb.len && !(sb.data[*pos] >= '0' && sb.data[*pos] <= '9')) {
        while (*pos < sb.len && sb.data[*pos] != '\n') {
            *pos += 1;
        }
        *pos += 1;
    } else {
        *pos -= path_len + 1;
    }

    return *pos;
}

LineRange error_extract_position(uint32_t *pos, StringBuilder sb, uint32_t path_len) {
    *pos += path_len + 1;

    uint32_t line_nr = 0;
    uint32_t position = 0;

    while (*pos < sb.len && sb.data[*pos] != ':' && is_numeric(sb.data[*pos])) {
        line_nr = (line_nr * 10) + sb.data[*pos] - '0';
        *pos += 1;
    }
    *pos += 1;

    while (*pos < sb.len && sb.data[*pos] != ':' && is_numeric(sb.data[*pos])) {
        position = (position * 10) + sb.data[*pos] - '0';
        *pos += 1;
    }
    *pos += 1;

    return (LineRange){
        .start = (Token){ .beg = position },
        .end_line = line_nr,
    };
}

void error_append_remaining_till_next(
        Arena *arena,
        uint32_t *pos, 
        StringBuilder sb, 
        StringBuilder *new_error,
        const char *unit_path, 
        uint32_t unit_path_len,
        uint32_t replace_line,
        uint32_t replace_with
) {
    uint32_t replace_with_len = string_len_of_number(replace_with) + 10;
    uint32_t put_left_of_pipe = 0;

    bool is_next_error = true;
    bool is_left_of_pipe = false;

    while (*pos + unit_path_len < sb.len) {
        is_next_error = true;

        // TODO: better skipping
        for (uint32_t i = 0; i < unit_path_len; i++) {
            if (sb.data[*pos + i] != unit_path[i]) {
                is_next_error = false;
                break;
            }
        }

        if (is_next_error) return;
        if (!is_left_of_pipe) {
            if (sb.data[*pos] == '\n') {
                is_left_of_pipe = true;
                put_left_of_pipe = 0;
            }

            *push(new_error, arena) = sb.data[*pos];
            *pos += 1;
            continue;
        }

        uint32_t number = 0;

        while (*pos < sb.len && sb.data[*pos] >= '0' && sb.data[*pos] <= '9') {
            number *= 10;
            number += sb.data[*pos] - '0';
            *pos += 1;
        }

        if          (number > 0) {
            uint32_t adjusted_number = replace_with + number - replace_line;
            put_left_of_pipe += string_len_of_number(adjusted_number);
            append_number(arena, new_error, adjusted_number);
        } else if   (sb.data[*pos] == '|') {
            for (uint32_t i = 0; i < replace_with_len - put_left_of_pipe; i++) {
                *push(new_error, arena) = ' ';
            }

            is_left_of_pipe = false;
            put_left_of_pipe = 0;
            *push(new_error, arena) = sb.data[*pos];
            put_left_of_pipe += 1;
        } else {
            *push(new_error, arena) = sb.data[*pos];
            put_left_of_pipe += 1;
        } 

        *pos += 1;
    }

    while (*pos < sb.len) {
        if (is_left_of_pipe && sb.data[*pos] == '|') {
            for (uint32_t i = 0; i < replace_with_len - put_left_of_pipe; i++) {
                *push(new_error, arena) = ' ';
            }
            is_left_of_pipe = false;
        }

        *push(new_error, arena) = sb.data[*pos];
        *pos += 1;
        put_left_of_pipe += 1;
    }
}

CopyPosition error_search_range(uint32_t line, uint32_t start, uint32_t end, FileSections *fs) {
    // TODO: replace with binary search

    for (uint32_t i = start; i < end; i++) {
        CopyPosition cp = fs->positions.data[i];

        if (cp.start_line <= line && line <= cp.end_line) {
            return cp;
        }
    }

    printf("Error: this is a bug in mob please report it -> `line=%d`\n", line);
    exit(1);
}

CopyPosition error_get_correct_location(uint32_t line, FileSections *fs) {
    if          (is_include_error(line, fs)) {
        return error_search_range(line, 0, fs->include_arr, fs);
    } else if   (is_define_error(line, fs)) {
        return error_search_range(line, fs->include_arr, fs->define_arr, fs);
    } else if   (is_pragma_error(line, fs)) {
        return error_search_range(line, fs->define_arr, fs->pragma_arr, fs);
    } else if   (is_typedef_error(line, fs)) {
        return error_search_range(line, fs->pragma_arr, fs->typedef_arr, fs);
    } else if   (is_enum_error(line, fs)) {
        return error_search_range(line, fs->typedef_arr, fs->enum_arr, fs);
    } else if   (is_struct_error(line, fs)) {
        return error_search_range(line, fs->enum_arr, fs->struct_arr, fs);
    } else if   (is_comp_if_error(line, fs)) {
        return error_search_range(line, fs->struct_arr, fs->compiler_if_arr, fs);
    } else if   (is_global_var_error(line, fs)) {
        return error_search_range(line, fs->compiler_if_arr, fs->global_variable_arr, fs);
    } else if   (is_function_error(line, fs)) {
        return error_search_range(line, fs->global_variable_arr, fs->function_arr, fs);
    } else {
        printf("Error: encountered unknown error state line=%d\n", line);
        exit(1);
    }
}

uint32_t error_append_original_location(
        Arena *arena,
        StringBuilder *sb,
        StringBuilder *err_msg, 
        const char **paths, 
        uint32_t unit_path_len,
        uint32_t line, 
        uint32_t position,
        uint32_t start,
        uint32_t end,
        FileSections *fs
) {
    CopyPosition mapped_error_location = error_get_correct_location(line, fs);
    uint32_t off = line - mapped_error_location.start_line - 1;
    uint32_t line_nr = mapped_error_location.file_line + off;
    if (line_nr == 0) {
        line_nr = 1;
    }

    if (start != end) {
        append_string(arena, err_msg, paths[mapped_error_location.file]);
        for (uint32_t i = start + unit_path_len; i < end; i++) {
            *push(err_msg, arena) = sb->data[i];
        }
    }

    append_string(arena, err_msg, paths[mapped_error_location.file]);
    *push(err_msg, arena) = ':';
    append_number(arena, err_msg, line_nr);
    *push(err_msg, arena) = ':';
    append_number(arena, err_msg, position);
    *push(err_msg, arena) = ':';

    return line_nr;
}

void concat_compile_command(
        const char *compiler_path, 
        const char **flags,
        uint32_t flags_len,
        const char *c_source_path,
        uint32_t c_source_path_len,
        char *cmd
) {
    uint32_t cmd_pos = strlen(compiler_path);

    for (uint32_t i = 0; i < cmd_pos; i++) {
        cmd[i] = compiler_path[i];
    }
    cmd[cmd_pos] = ' ';
    cmd_pos += 1;

    for (uint32_t i = 0; i < flags_len; i++) {
        uint32_t ith_flag_len = strlen(flags[i]);
        uint32_t pos = 0;

        while (pos < ith_flag_len) {
            cmd[cmd_pos + pos] = flags[i][pos];
            pos += 1;
        }

        cmd_pos += ith_flag_len;
        cmd[cmd_pos] = ' ';
        cmd_pos += 1;
    }

    for (uint32_t i = 0; i < c_source_path_len; i++) {
        cmd[cmd_pos + i] = c_source_path[i];
    }

    // NOTE: appending executable name to be unit path without ending
    cmd_pos += c_source_path_len;
    cmd[cmd_pos++] = ' ';
    cmd[cmd_pos++] = '-';
    cmd[cmd_pos++] = 'o';
    cmd[cmd_pos++] = ' ';

    // TODO: could be asserted [SIZE, having a .c ending]
    for (uint32_t i = 0; i < c_source_path_len - 2; i++) {
        cmd[cmd_pos + i] = c_source_path[i];
    }

    // NOTE: appending redirect stderr to stdout
    cmd_pos += c_source_path_len - 2;
    cmd[cmd_pos] = ' ';
    cmd_pos += 1;

    const char *redirect = "2>&1";
    uint32_t redirect_len = strlen(redirect);

    for (uint32_t i = 0; i < redirect_len; i++) {
        cmd[cmd_pos + i] = redirect[i];
    }
}

void mob_compile(
        Arena *arena, 
        const char *unit_path, 
        uint32_t unit_path_len, 
        const char **flags,
        uint32_t flags_len,
        const char **paths,
        FileSections *fs
) {
    uint32_t pos = 0;
    char cmd[4096] = {0};
    char buf[1024] = {0};
    StringBuilder sb = {0};
    StringBuilder error_msg =  {0};
    const char *compiler = "/usr/bin/cc";
    concat_compile_command(compiler, flags, flags_len, unit_path, unit_path_len, cmd);
    printf("%s\n", cmd);

    #ifdef _WIN32
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        bool res = CreateProcessA("./build.bat", "", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);
        if (res == false) {
            printf("error %d\n", GetLastError());
        }
    #endif

    FILE *out = popen(cmd, "r");
    if (out == NULL) {
        printf("ERROR: cannot execute command\n");
        exit(1);
    } 

    while (fgets(buf, sizeof(buf), out) != NULL) {
        append_string(arena, &sb, buf);
    }
    pclose(out);

    while (pos < sb.len) {
        uint32_t start = pos;
        uint32_t end = error_skip_if_not_location(&pos, sb, unit_path_len);
        LineRange lr = error_extract_position(&pos, sb, unit_path_len);

        uint32_t line_nbr = lr.end_line;
        uint32_t position = lr.start.beg;

        if (is_duplicated_error(line_nbr, fs)) {
            error_skip_to_next(&pos, sb, unit_path, unit_path_len);
            continue;
        }

        uint32_t new_line = error_append_original_location(
                arena, 
                &sb, 
                &error_msg, 
                paths, 
                unit_path_len,
                line_nbr, 
                position, 
                start, 
                end, 
                fs);
        error_append_remaining_till_next(arena, &pos, sb, &error_msg, unit_path, unit_path_len, line_nbr, new_line);
    }

    if (sb.len == 0) {
        printf("[INFO]: compiled -> `%s`\n", unit_path);
    } else {
        printf("%s\n", error_msg.data);
    }
}

