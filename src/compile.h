#ifndef COMPILE_H_
#define COMPILE_H_

#if !MOB_SELF_BUILD
    #include "parser.h"
    #include "writer.h"
#endif

void mob_compile(Arena *arena, const char *path, uint32_t path_len, FileSections fs, ArrayFileContent content);

#endif /* COMPILE_H_ */
