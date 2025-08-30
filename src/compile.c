#include <stdio.h>
#include <stdlib.h>

#if !GOC_SELF_BUILD
    #include "compile.h"
#endif

void goc_compile(const char *path) {
    /*
    FILE *process = popen("/usr/bin/gcc", "r");
    if (process == NULL) {
        printf("ERROR\n");
        return;
    } 

    char buf[1024];

    while (fgets(buf, sizeof(buf), process) != NULL) {
        printf("%s\n", buf);
    } 

    pclose(process);
    */
}
