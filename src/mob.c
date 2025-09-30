#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
	#include <windows.h>
#elif __unix__
	#include <sys/mman.h>
#endif

#if !MOB_SELF_BUILD
    #include "arena.h"
    #include "parser.h"
    #include "writer.h"
    #include "compile.h"
#endif

#define UNIT_PATH       "app.c"
#define UNIT_PATH_LEN   sizeof(UNIT_PATH) / sizeof(UNIT_PATH[0]) - 1

static const char *FLAGS[] = {
    "-Wall",
    "-Wextra",
    "-DMOB_SELF_BUILD=1",
};
static const uint32_t FLAG_COUNT = sizeof(FLAGS) / sizeof(FLAGS[0]);

// TODO: think about recursive directory 
// decent to find other c files
static const char *PATHS[] = {
    "src/mob.c",
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
};
static const uint32_t PATH_COUNT = sizeof(PATHS) / sizeof(PATHS[0]);

void allocate_arenas(Arena *app, Arena *files, ptrdiff_t file_buffer_size) {
#if _WIN32
	app->beg = VirtualAlloc(NULL, file_buffer_size, MEM_COMMIT, PAGE_READWRITE);
    app->end = app->beg + file_buffer_size; 

	files->beg = VirtualAlloc(NULL, file_buffer_size, MEM_COMMIT, PAGE_READWRITE);
    files->end = files->beg + file_buffer_size; 
#elif __unix__
	app->beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    app->end = app->beg + file_buffer_size; 

    files->beg = mmap(NULL, file_buffer_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    files->end = files->beg + file_buffer_size; 
#endif
}

int main(void) {
    ptrdiff_t file_buffer_size = PATH_COUNT * 1024 * 1024;

    Arena app   = {0};
    Arena files = {0};
    allocate_arenas(&app, &files, file_buffer_size);

    ArrayFileContent contents = {0}; 
    ArrayCharPtr file_starts = {0};

    // NOTE: could be multithreaded
    for (uint32_t i = 0; i < PATH_COUNT; i++) {
        printf("[INFO]: proccessing -> `%s`\n", PATHS[i]);
        *push(&file_starts, &app) = files.beg;
        *push(&contents, &app) = parse_c_file(&app, &files, PATHS[i]);
    }

    FileSections fs = write_file(&app, UNIT_PATH, file_starts, contents);
    mob_compile(&app, UNIT_PATH, UNIT_PATH_LEN, FLAGS, FLAG_COUNT, PATHS, &fs);

    return 0;
}
