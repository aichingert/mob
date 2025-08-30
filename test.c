#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define CONST 10

#if true

#endif 
#if true
#define ZT 10
#endif

#define MAX(a, b) (a) > (b) \
    ? \
    (a) \
    : \
    (b) \

// ashdlkl */

struct String {
    const char *val;
    const long len;
};

struct HelloData {
    const char *this_is_it;
};

typedef enum TokenType {
    T_ENUM      = 0,
    T_STRUCT    = 1,
    T_TYPEDEF   = 2,
    T_IDENT     = 3,
    C_PRAGMA    = 4,
    C_DEFINE    = 5,
    C_INCLUDE   = 6,

    O_EQ        = 8,
    TT_L_BRACE  = 9,
    TT_R_BRACE  = 10,
    TT_L_PAREN  = 11,
    TT_R_PAREN  = 12,
    TT_L_BRACKET= 13,
    TT_R_BRACKET= 14,
    TT_COMMA    = 15,
    TT_SEMICOLON= 16,
    TT_BACKSLASH= 17,

    R_EOF       = 30,
    R_IGNORE    = 31,
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



typedef enum VALUES {
    A, B, C, D = 10,
} VALUES;

typedef int Zahl;
typedef bool Boolean;

static char *hello() {
    return "dshflasj";
}

static const char h = { 10 };


int main(void) {
    printf("%d %s %d\n", h, hello(), ZT);

    VALUES a = D;
    return 0;

}

