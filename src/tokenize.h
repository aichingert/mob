#ifndef TOKENIZE_H_
#define TOKENIZE_H_

#if !MOB_SELF_BUILD
    #include "arena.h"
#endif

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

#endif /* TOKENIZE_H_ */

