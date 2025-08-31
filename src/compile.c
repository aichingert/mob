#include <stdio.h>
#include <stdlib.h>

#if !GOC_SELF_BUILD
    #include "compile.h"
#endif

void concat(const char *a, const char *b, char *buf) {
    while (*a != '\0') *buf++ = *a++;
    while (*b != '\0') *buf++ = *b++;
}

void goc_compile(const char *path, FileSections fs, ArrayFileContent content) {
    char cmd[1024] = {0};
    concat("/usr/bin/cc -DGOC_SELF_BUILD=1 ", path, cmd);

    printf("%s\n", cmd);
    FILE *process = popen(cmd, "r");
    if (process == NULL) {
        printf("ERROR\n");
        return;
    } 

    char buf[1024];

    while (fgets(buf, sizeof(buf), process) != NULL) {
        printf("%s\n", buf);
    } 

    pclose(process);
}
