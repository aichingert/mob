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

void write_file(Arena *arena, Arena *files, const char *create_path, ArrayCharPtr file_starts, ArrayFileContent contents);

#endif /* WRITER_H_ */

