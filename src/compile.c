#include <stdio.h>
#include <stdlib.h>

#if !MOB_SELF_BUILD
    #include "arena.h"
    #include "writer.h"
    #include "compile.h"
#endif


void concat_with_space(const char **s, uint32_t s_len, char *buf) {
    for (uint32_t i = 0; i < s_len; i++) {
        while (*s[i] != '\0') *buf++ = *s[i]++;
        *buf++ = ' ';
    }
}

bool is_duplicated_error(uint32_t line, FileSections fs) {
    return line >= fs.enum_end && line < fs.struct_declare_end
        || line >= fs.global_variable_end && line < fs.function_header_end;
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
}

LineRange error_extract_position(uint32_t *pos, StringBuilder sb, uint32_t path_len) {
    *pos += path_len + 1;

    uint32_t line_nr = 0;
    uint32_t position = 0;

    while (*pos < sb.len && sb.data[*pos] != ':') {
        line_nr = (line_nr * 10) + sb.data[*pos] - '0';
        *pos += 1;
    }
    *pos += 1;

    while (*pos < sb.len && sb.data[*pos] != ':') {
        position = (position * 10) + sb.data[*pos] - '0';
        *pos += 1;
    }
    *pos += 1;

    return (LineRange){
        .start = (Token){ .beg = position },
        .end_line = line_nr,
    };
}

void mob_compile(
        Arena *arena, 
        const char *path, 
        uint32_t path_len, 
        FileSections fs, 
        ArrayFileContent content
) {
    char cmd[1024] = {0};
    const char *strs[] = {
        "/usr/bin/cc -Wextra -Wall -fsanitize=leak -DMOB_SELF_BUILD=1", 
        path, 
        "2>&1"
    };
    concat_with_space(strs, sizeof(strs) / sizeof(strs[0]), cmd);

    char buf[1024];
    StringBuilder sb = {0};
    StringBuilder error_msg =  {0};
    FILE *out = popen(cmd, "r");

    if (out == NULL) {
        printf("ERROR: cannot execute command\n");
        exit(1);
    } 

    while (fgets(buf, sizeof(buf), out) != NULL) {
        append_string(arena, &sb, buf);
    }
    pclose(out);

    uint32_t pos = 0;

    while (pos < sb.len) {
        LineRange lr = error_extract_position(&pos, sb, path_len);

        uint32_t line_nbr = lr.end_line;
        uint32_t position = lr.start.beg;

        printf("line:%d - pos:%d \n", line_nbr, position);

        if (is_duplicated_error(line_nbr, fs)) {
            error_skip_to_next(&pos, sb, path, path_len);
            continue;
        }

        error_skip_to_next(&pos, sb, path, path_len);


    }

}
