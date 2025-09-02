#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#if !MOB_SELF_BUILD
    #include "parser.h"
    #include "tokenize.h"
#endif


// ( | =
// ( -> {, =, ;
//int (*f)(int, int) = xyz;
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

    char *buf = alloc(arena, char, *len, false);
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

bool is_compiler_if_intrinsic(TokenType type) {
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
        TokenType type = toks.data[pos].type;

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
