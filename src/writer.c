#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !GOC_SELF_BUILD
    #include "writer.h"
    #include "tokenize.h"
#endif

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
