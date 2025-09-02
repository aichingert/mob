#ifndef COMPILE_H_
#define COMPILE_H_

#if !MOB_SELF_BUILD
    #include "parser.h"
    #include "writer.h"
#endif

void mob_compile(
        Arena *arena, 
        const char *unit_path, 
        uint32_t unit_path_len, 
        const char **paths, 
        FileSections *fs,
        ArrayFileContent content
);

#endif /* COMPILE_H_ */
