#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !GOC_SELF_BUILD
    #include "writer.h"
    #include "tokenize.h"
#endif

void write_string(Arena *arena, StringBuilder *sb, const char *str) {
    for (uint32_t i = 0; i < strlen(str); i++) {
        *push(sb, arena) = str[i];
    }
}

void write_file(Arena *arena, Arena *files, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents) {

    FILE *unit_file = fopen(create_path, "w");
    if (unit_file == NULL) {
        printf("Error: unable to create `%create_path`\n", create_path);
        exit(1);
    }

    StringBuilder sb = {0};

    // NOTE: begin with putting all includes
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayLineRange includes = contents.data[i].includes;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < includes.len; j++) {
            uint32_t pos    = includes.data[j].start.beg;
            uint32_t line   = includes.data[j].start.line;
            uint32_t end_line = includes.data[j].end_line;

            while (line <= end_line) {
                if (file[pos] == '\n') line++;
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
        }
    }
    *push(&sb, arena) = '\n';

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayLineRange defines = contents.data[i].defines;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < defines.len; j++) {
            uint32_t pos    = defines.data[j].start.beg;
            uint32_t line   = defines.data[j].start.line;
            uint32_t end_line = defines.data[j].end_line;

            while (line <= end_line) {
                if (file[pos] == '\n') line++;
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            *push(&sb, arena) = '\n';
        }
    }

    // TODO: include pragmas
 
    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange enums = contents.data[i].enums;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < enums.len; j++) {
            uint32_t pos = enums.data[j].start.beg;
            uint32_t end = enums.data[j].end_char.beg;

            write_string(arena, &sb, "typedef enum ");

            while (pos < end) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }

            pos = enums.data[j].start.beg;
            while (is_ident(file[pos])) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            write_string(arena, &sb, ";\n\n");
        }
    }

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            uint32_t pos = structs.data[j].start.beg;
            write_string(arena, &sb, "typedef struct ");

            while (is_ident(file[pos])) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }

            *push(&sb, arena) = ' ';
            pos = structs.data[j].start.beg;

            while (is_ident(file[pos])) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            write_string(arena, &sb, ";\n");
        }
    }

    *push(&sb, arena) = '\n';

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange structs = contents.data[i].structs;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < structs.len; j++) {
            uint32_t pos = structs.data[j].start.beg;
            uint32_t end = structs.data[j].end_char.beg;

            write_string(arena, &sb, "typedef struct ");

            while (pos < end) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }

            *push(&sb, arena) = ' ';
            pos = structs.data[j].start.beg;

            while (is_ident(file[pos])) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            write_string(arena, &sb, ";\n\n");
        }
    }

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayLineRange cifs = contents.data[i].compiler_ifs;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < cifs.len; j++) {
            uint32_t pos    = cifs.data[j].start.beg;
            uint32_t line   = cifs.data[j].start.line;
            uint32_t end_line = cifs.data[j].end_line;

            while (line <= end_line) {
                if (file[pos] == '\n') line++;
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            *push(&sb, arena) = '\n';
        }
    }

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange globals = contents.data[i].globals;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < globals.len; j++) {
            uint32_t pos = globals.data[j].start.beg;
            uint32_t end = globals.data[j].end_char.beg;

            while (pos < end) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            write_string(arena, &sb, ";\n");
        }
    }
    *push(&sb, arena) = '\n'; 

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;

            while (file[pos] != '{') {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }
            sb.data[sb.len - 1] = ';';
            *push(&sb, arena) = '\n';
        }
    }

    *push(&sb, arena) = '\n';

    for (uint32_t i = 0; i < contents.len; i++) {
        ArrayCharRange funcs = contents.data[i].functions;
        const char *file = file_starts.data[i];

        for (uint32_t j = 0; j < funcs.len; j++) {
            uint32_t pos = funcs.data[j].start.beg;
            uint32_t end = funcs.data[j].end_char.beg;

            while (pos <= end) {
                *push(&sb, arena) = file[pos];
                pos += 1;
            }

            write_string(arena, &sb, "\n\n");
        }
    }

    fwrite(sb.data, sizeof(sb.data[0]), sb.len, unit_file);
    fclose(unit_file);
}
