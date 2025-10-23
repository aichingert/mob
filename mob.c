#include <stdio.h>

char **ENV;

s32 main(s32 argc, const char **argv, char **environ) {
    (void)argc;
    (void)argv;
    ENV = environ;

    Arena app = {0};
    arena_init(&app, (u64)2 << 20); 

    arena_deinit(&app);
    return 0;
}

