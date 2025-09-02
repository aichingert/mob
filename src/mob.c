#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#if !MOB_SELF_BUILD
    #include "arena.h"
    #include "parser.h"
    #include "writer.h"
    #include "compile.h"
#endif

#define UNIT_PATH       "mob_unit.c"
#define UNIT_PATH_LEN   sizeof(UNIT_PATH) / sizeof(UNIT_PATH[0]) - 1

static const char *PATHS[] = {
    /*
    "src/MOB.c",
    "src/arena.c",
    "src/arena.h",
    "src/tokenize.c",
    "src/tokenize.h",
    "src/parser.c",
    "src/parser.h",
    "src/writer.h",
    "src/writer.c",
    "src/compile.c",
    "src/compile.h",
    */
    "example/app.c",
    "example/math.c",
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

    FileSections fs = write_file(&app, UNIT_PATH, file_starts, contents);

    printf("include:\t%d\n", fs.include_end);
    printf("define:\t%d\n", fs.define_end);
    printf("pragma:\t%d\n", fs.pragma_end);

    printf("enums:\t%d\n", fs.enum_end);
    printf("struct-dec:\t%d\n", fs.struct_declare_end);
    printf("struct-def:\t%d\n", fs.struct_define_end);
    printf("compiler if:\t%d\n", fs.compiler_if_end);
    printf("global var:\t%d\n", fs.global_variable_end);
    printf("func-head:\t%d\n", fs.function_header_end);
    printf("func-def:\t%d\n", fs.function_define_end);    

    // TODO: store errors
    mob_compile(&app, UNIT_PATH, UNIT_PATH_LEN, PATHS, &fs, contents);

    return 0;
}
