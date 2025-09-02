#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !MOB_SELF_BUILD
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

    // APPENDING INCLUDES
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].includes, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.include_end = line_nr;
    fs.include_arr = fs.positions.len;

    // APPENDING DEFINES
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].defines, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.define_end = line_nr;
    fs.define_arr = fs.positions.len;

    // APPENDING PRAGMAS
    for (uint32_t i = 0; i < contents.len; i++) {
        append_line_ranges(arena, &sb, &line_nr, contents.data[i].pragmas, file_starts.data[i], i, &fs);
    }
    append_newline(arena, &sb, &line_nr);
    fs.pragma_end = line_nr;
    fs.pragma_arr = fs.positions.len;
 
    // APPENDING ENUMS
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
