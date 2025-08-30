#include <stdio.h>
#include <assert.h>
#include <string.h>

#if !GOC_SELF_BUILD
    #include "tokenize.h"
#endif

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
