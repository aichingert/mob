#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#if !GOC_SELF_BUILD
    #include "arena.h"
    #include "parser.h"
    #include "writer.h"
#endif

#define UNIT_PATH "goc_unit.c"

static const char *PATHS[] = {
    "src/goc.c",
    "src/arena.c",
    "src/arena.h",
    "src/tokenize.c",
    "src/tokenize.h",
    "src/parser.c",
    "src/parser.h",
    "src/writer.h",
    "src/writer.c",
};
static const uint32_t PATH_COUNT = sizeof(PATHS) / sizeof(PATHS[0]);

int main(int argc, char **argv) {
    ptrdiff_t file_buffer_size = PATH_COUNT * 1024 * 1024;

    Arena app, files = {0};
    app.beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    app.end = app.beg + file_buffer_size; 

    files.beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    files.end = files.beg + file_buffer_size; 

    ArrayFileContent contents = {0}; 
    ArrayCharPtr file_starts = {0};

    // NOTE: could be multithreaded
    for (uint32_t i = 0; i < PATH_COUNT; i++) {
        printf("Proccessing: `%s`\n", PATHS[i]);
        *push(&file_starts, &app) = files.beg;
        *push(&contents, &app) = parse_c_file(&app, &files, PATHS[i]);
    }

    write_file(&app, UNIT_PATH, file_starts, contents);
    return 0;
}
