#ifndef PARSER_H_
#define PARSER_H_

#if !GOC_SELF_BUILD
    #include "arena.h"
    #include "tokenize.h"
#endif

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

#endif /* PARSER_H_ */
