#ifndef WRITER_H_
#define WRITER_H_

#if !MOB_SELF_BUILD
    #include "parser.h"
#endif

// NOTE: maybe improve this if it leads to bottlenecks
typedef struct StringBuilder {
    char *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} StringBuilder;

typedef struct CopyPosition {
    uint32_t file;
    uint32_t file_line;
    uint32_t start_line;
    uint32_t end_line;
} CopyPosition;

typedef struct ArrayCopyPosition {
    CopyPosition *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} ArrayCopyPosition;

typedef struct FileSections {
    uint32_t include_end;
    uint32_t include_arr;
    uint32_t define_end;
    uint32_t define_arr;
    uint32_t pragma_end;
    uint32_t pragma_arr;
    uint32_t typedef_end;
    uint32_t typedef_arr;
    uint32_t enum_end;
    uint32_t enum_arr;
    uint32_t struct_declare_end;
    uint32_t struct_define_end;
    uint32_t struct_arr;
    uint32_t compiler_if_end;
    uint32_t compiler_if_arr;
    uint32_t function_header_end;
    uint32_t global_variable_end;
    uint32_t global_variable_arr;
    uint32_t function_define_end;
    uint32_t function_arr;

    ArrayCopyPosition positions;
} FileSections;

uint32_t append_string(Arena *arena, StringBuilder *sb, const char *str);
void append_number(Arena *arena, StringBuilder *sb, int64_t number);
FileSections write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

#endif /* WRITER_H_ */

