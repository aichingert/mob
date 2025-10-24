#include <stdio.h>

char **ENV = NULL;

static const String BUILD = S("app.c");
static const String FLAGS[] = {
    S("-std=c23"),
    S("-ffreestanding"),
    S("-Wextra"),
    S("-Wall"),
};
static const String PATHS[] = {
    S("example/app.c"),
    S("example/math.c"),
};
static const u32    PATH_LEN = sizeof(PATHS) / sizeof(PATHS[0]);

s32 main(s32 argc, const char **argv, char **environ) {
    // TODO: maybe add flags
    (void)argc;
    (void)argv;
    ENV = environ;
    Arena app = {0};
    arena_init(&app, 2 << 20);

    for (u32 i = 0; i < PATH_LEN; i++) {
        s32 file = os_open_file(PATHS[i], O_RDWR, 644);
        s64 size = os_file_size(file);
        u8 *mem = alloc(&app, u8, size);

        printf("result: %d\n", os_read(file, mem, size));

        for (int i = 0; i < size; i++) {
            printf("%c", mem[i]);
        }
        printf("\n");

        printf("%d - %s %ld\n", file, PATHS[i].val, os_file_size(file));

        os_close_file(file);
    }

    arena_deinit(&app);
    return 0;
}

