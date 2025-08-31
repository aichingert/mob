#ifndef COMPILE_H_
#define COMPILE_H_

#if !GOC_SELF_BUILD
    #include "parser.h"
    #include "writer.h"
#endif

void goc_compile(const char *path, FileSections fs, ArrayFileContent content);

#endif /* COMPILE_H_ */
