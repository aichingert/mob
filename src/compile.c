#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !MOB_SELF_BUILD
    #include "arena.h"
    #include "writer.h"
    #include "compile.h"
#endif

uint32_t reverse_number(uint32_t number) {
    uint32_t reverse = 0;

    while (number > 0) {
        reverse *= 10;
        reverse += number % 10;
        number /= 10;
    }

    return reverse;
}

uint32_t string_len_of_number(uint32_t number) {
    uint32_t len = 0;

    while (number > 0) {
        number /= 10;
        len += 1;
    }

    return len == 0 ? len + 1 : len;
}

void concat_with_space(const char **s, uint32_t s_len, char *buf) {
    for (uint32_t i = 0; i < s_len; i++) {
        while (*s[i] != '\0') *buf++ = *s[i]++;
        *buf++ = ' ';
    }
}

bool is_duplicated_error(uint32_t line, FileSections *fs) {
    return (line >= fs->enum_end        && line < fs->struct_declare_end)
        || (line >= fs->compiler_if_end && line < fs->function_header_end);
}

bool is_include_error(uint32_t line, FileSections *fs) {
    return line < fs->include_end && fs->include_arr > 0;
}

bool is_define_error(uint32_t line, FileSections *fs) {
    return line >= fs->include_end && line < fs->define_end && fs->define_arr - fs->include_arr > 0;
}

bool is_pragma_error(uint32_t line, FileSections *fs) {
    return line >= fs->define_end && line < fs->pragma_end;
}

bool is_enum_error(uint32_t line, FileSections *fs) {
    return line >= fs->pragma_end && line < fs->enum_end;
}

bool is_struct_error(uint32_t line, FileSections *fs) {
    return line >= fs->struct_declare_end && line < fs->struct_define_end;
}

bool is_comp_if_error(uint32_t line, FileSections *fs) {
    return line >= fs->struct_define_end && line < fs->compiler_if_end;
}

bool is_global_var_error(uint32_t line, FileSections *fs) {
    return line >= fs->function_header_end && line < fs->global_variable_end;
}

bool is_function_error(uint32_t line, FileSections *fs) {
    return line >= fs->global_variable_end && line < fs->function_define_end;
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

    *pos = sb.len;
}

uint32_t error_skip_in_function(uint32_t *pos, StringBuilder sb, uint32_t path_len) {
    *pos += path_len + 1;
    uint32_t advance = 0;
    const char *in_function = " In function";
    uint32_t len = strlen(in_function);

    while (*pos + advance < sb.len && sb.data[*pos + advance] == in_function[advance]) {
        advance += 1;
    }

    if (advance == len) {
        *pos += advance;
        while (*pos < sb.len && sb.data[*pos] != '\n') {
            *pos += 1;
        }
        *pos += 1;
    } else {
        *pos -= path_len + 1;
    }

    return *pos;
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

void error_append_remaining_till_next(
        Arena *arena,
        uint32_t *pos, 
        StringBuilder sb, 
        StringBuilder *new_error,
        const char *unit_path, 
        uint32_t unit_path_len,
        uint32_t replace_line,
        uint32_t replace_with
) {
    uint32_t replace_with_len = string_len_of_number(replace_with) + 10;
    uint32_t put_left_of_pipe = 0;

    bool is_next_error = true;
    bool is_left_of_pipe = false;

    while (*pos + unit_path_len < sb.len) {
        is_next_error = true;

        // TODO: better skipping
        for (uint32_t i = 0; i < unit_path_len; i++) {
            if (sb.data[*pos + i] != unit_path[i]) {
                is_next_error = false;
                break;
            }
        }

        if (is_next_error) return;
        if (!is_left_of_pipe) {
            if (sb.data[*pos] == '\n') {
                is_left_of_pipe = true;
                put_left_of_pipe = 0;
            }

            *push(new_error, arena) = sb.data[*pos];
            *pos += 1;
            continue;
        }

        uint32_t number = 0;

        while (*pos < sb.len && sb.data[*pos] >= '0' && sb.data[*pos] <= '9') {
            number *= 10;
            number += sb.data[*pos] - '0';
            *pos += 1;
        }

        if          (number > 0) {
            uint32_t adjusted_number = replace_with + number - replace_line;
            put_left_of_pipe += string_len_of_number(adjusted_number);
            append_number(arena, new_error, adjusted_number);
        } else if   (sb.data[*pos] == '|') {
            for (uint32_t i = 0; i < replace_with_len - put_left_of_pipe; i++) {
                *push(new_error, arena) = ' ';
            }

            is_left_of_pipe = false;
            put_left_of_pipe = 0;
            *push(new_error, arena) = sb.data[*pos];
            put_left_of_pipe += 1;
        } else {
            *push(new_error, arena) = sb.data[*pos];
            put_left_of_pipe += 1;
        } 

        *pos += 1;
    }

    while (*pos < sb.len) {
        if (is_left_of_pipe && sb.data[*pos] == '|') {
            for (uint32_t i = 0; i < replace_with_len - put_left_of_pipe; i++) {
                *push(new_error, arena) = ' ';
            }
            is_left_of_pipe = false;
        }

        *push(new_error, arena) = sb.data[*pos];
        *pos += 1;
        put_left_of_pipe += 1;
    }
}

CopyPosition error_search_range(uint32_t line, uint32_t start, uint32_t end, FileSections *fs) {
    // TODO: replace with binary search

    for (uint32_t i = start; i < end; i++) {
        CopyPosition cp = fs->positions.data[i];

        if (cp.start_line <= line && line <= cp.end_line) {
            return cp;
        }
    }

    printf("Error: this is a bug in mob please report it\n");
    exit(1);
}

CopyPosition error_get_correct_location(uint32_t line, FileSections *fs) {
    if          (is_include_error(line, fs)) {
        return error_search_range(line, 0, fs->include_arr, fs);
    } else if   (is_define_error(line, fs)) {
        return error_search_range(line, fs->include_arr, fs->define_arr, fs);
    } else if   (is_pragma_error(line, fs)) {
        return error_search_range(line, fs->define_arr, fs->pragma_arr, fs);
    } else if   (is_enum_error(line, fs)) {
        return error_search_range(line, fs->pragma_arr, fs->enum_arr, fs);
    } else if   (is_struct_error(line, fs)) {
        return error_search_range(line, fs->enum_arr, fs->struct_arr, fs);
    } else if   (is_comp_if_error(line, fs)) {
        return error_search_range(line, fs->struct_arr, fs->compiler_if_arr, fs);
    } else if   (is_global_var_error(line, fs)) {
        return error_search_range(line, fs->compiler_if_arr, fs->global_variable_arr, fs);
    } else if   (is_function_error(line, fs)) {
        return error_search_range(line, fs->global_variable_arr, fs->function_arr, fs);
    } else {
        printf("Error: encountered unknown error state line=%d\n", line);
        exit(1);
    }
}

uint32_t error_append_original_location(
        Arena *arena,
        StringBuilder *sb,
        StringBuilder *err_msg, 
        const char **paths, 
        uint32_t unit_path_len,
        uint32_t line, 
        uint32_t position,
        uint32_t start,
        uint32_t end,
        FileSections *fs
) {
    CopyPosition mapped_error_location = error_get_correct_location(line, fs);
    uint32_t off = line - mapped_error_location.start_line - 1;
    uint32_t line_nr = mapped_error_location.file_line + off;
    if (line_nr == 0) {
        line_nr = 1;
    }

    if (start != end) {
        append_string(arena, err_msg, paths[mapped_error_location.file]);
        for (uint32_t i = start + unit_path_len; i < end; i++) {
            *push(err_msg, arena) = sb->data[i];
        }
    }

    append_string(arena, err_msg, paths[mapped_error_location.file]);
    *push(err_msg, arena) = ':';
    append_number(arena, err_msg, line_nr);
    *push(err_msg, arena) = ':';
    append_number(arena, err_msg, position);
    *push(err_msg, arena) = ':';

    return line_nr;
}

void mob_compile(
        Arena *arena, 
        const char *unit_path, 
        uint32_t unit_path_len, 
        const char **paths,
        FileSections *fs
) {
    char cmd[1024] = {0};
    const char *strs[] = {
        "/usr/bin/clang -Wextra -Wall -fsanitize=leak -DMOB_SELF_BUILD=1", 
        unit_path, 
        "2>&1"
    };
    concat_with_space(strs, sizeof(strs) / sizeof(strs[0]), cmd);

    char buf[1024];
    uint32_t pos = 0;
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

    while (pos < sb.len) {
        uint32_t start = pos;
        uint32_t end = error_skip_in_function(&pos, sb, unit_path_len);
        LineRange lr = error_extract_position(&pos, sb, unit_path_len);

        uint32_t line_nbr = lr.end_line;
        uint32_t position = lr.start.beg;

        if (is_duplicated_error(line_nbr, fs)) {
            error_skip_to_next(&pos, sb, unit_path, unit_path_len);
            continue;
        }

        uint32_t new_line = error_append_original_location(
                arena, 
                &sb, 
                &error_msg, 
                paths, 
                unit_path_len,
                line_nbr, 
                position, 
                start, 
                end, 
                fs);
        error_append_remaining_till_next(arena, &pos, sb, &error_msg, unit_path, unit_path_len, line_nbr, new_line);
    }

    if (sb.len == 0) {
        printf("[INFO]: compiled -> `%s`\n", unit_path);
    } else {
        printf("%s\n", error_msg.data);
    }
}
