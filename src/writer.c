#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !GOC_SELF_BUILD
    #include "writer.h"
    #include "tokenize.h"
#endif

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

uint32_t append_line_ranges(
        Arena *arena,
        StringBuilder *sb,
        ArrayLineRange alr,
        const char *source
) {
    uint32_t lines = 0;

    for (uint32_t i = 0; i < alr.len; i++) {
        lines += append_line_range(arena, sb, alr.data[i], source);
    }

    return lines;
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

FileSections write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents) {
    FILE *unit_file = fopen(create_path, "w");
    if (unit_file == NULL) {
        printf("Error: unable to create `%s`\n", create_path);
        exit(1);
    }

    uint32_t line_nr = 0;
    FileSections fs = {0};
    StringBuilder sb = {0};

    // APPENDING INCLUDES
    for (uint32_t i = 0; i < contents.len; i++) {
        line_nr += append_line_ranges(arena, &sb, contents.data[i].includes, file_starts.data[i]);
    }
    append_newline(arena, &sb, &line_nr);
    fs.include_end = line_nr;

    // APPENDING DEFINES
    for (uint32_t i = 0; i < contents.len; i++) {
        line_nr += append_line_ranges(arena, &sb, contents.data[i].defines, file_starts.data[i]);
    }
    append_newline(arena, &sb, &line_nr);
    fs.define_end = line_nr;

    // APPENDING PRAGMAS
    for (uint32_t i = 0; i < contents.len; i++) {
        line_nr += append_line_ranges(arena, &sb, contents.data[i].pragmas, file_starts.data[i]);
    }
    append_newline(arena, &sb, &line_nr);
    fs.pragma_end = line_nr;
 
    // APPENDING ENUMS
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange enums = contents.data[i].enums;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < enums.len; j++) {
            append_string(arena, &sb, "typedef enum ");
            line_nr += append_char_range(arena, &sb, enums.data[j], source);
            append_ident(arena, &sb, enums.data[j], source);
            line_nr += append_string(arena, &sb, ";\n\n");
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.enum_end = line_nr;

    // APPENDING STRUCTS
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

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            append_string(arena, &sb, "typedef struct ");
            line_nr += append_char_range(arena, &sb, structs.data[j], source);
            *push(&sb, arena) = ' ';
            append_ident(arena, &sb, structs.data[j], source);
            line_nr += append_string(arena, &sb, ";\n\n");
        }
    }
    fs.struct_define_end = line_nr;

    // APPENDING compiler ifs
    for (uint32_t i = 0; i < contents.len; i++) {
        line_nr += append_line_ranges(arena, &sb, contents.data[i].compiler_ifs, file_starts.data[i]);
    }
    append_newline(arena, &sb, &line_nr);
    fs.compiler_if_end = line_nr;

    // APPENDING globals
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange globals = contents.data[i].globals;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < globals.len; j++) {
            line_nr += append_char_range(arena, &sb, globals.data[j], source);
            line_nr += append_string(arena, &sb, ";\n");
        }
    }
    append_newline(arena, &sb, &line_nr);
    fs.global_variable_end = line_nr;

    // APPEND functions
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

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *source = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;
            uint32_t end = funcs.data[j].end_char.beg;

            while (pos <= end) {
                if (source[pos] == '\n') line_nr += 1;
                *push(&sb, arena) = source[pos];
                pos += 1;
            }
            line_nr += append_string(arena, &sb, "\n\n");
        }
    }
    fs.function_define_end = line_nr;

    fwrite(sb.data, sizeof(sb.data[0]), sb.len, unit_file);
    fclose(unit_file);

    return fs;
}
