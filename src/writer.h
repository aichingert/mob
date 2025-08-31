#ifndef WRITER_H_
#define WRITER_H_

#if !GOC_SELF_BUILD
    #include "parser.h"
#endif

// NOTE: maybe improve this if it leads to bottlenecks
typedef struct StringBuilder {
    char *data;
    ptrdiff_t len;
    ptrdiff_t cap;
} StringBuilder;

typedef struct FileSections {
    uint32_t include_end;
    uint32_t define_end;
    uint32_t pragma_end;
    uint32_t enum_end;
    uint32_t struct_declare_end;
    uint32_t struct_define_end;
    uint32_t compiler_if_end;
    uint32_t global_variable_end;
    uint32_t function_header_end;
    uint32_t function_define_end;
} FileSections;

FileSections write_file(Arena *arena, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

#endif /* WRITER_H_ */

