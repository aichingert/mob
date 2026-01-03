#include <stdio.h>

static bool   is_log_enabled = false;
static const String BUILD = S("bootstrap.c");

#define IS_STRING_EQL(string, slice)        \
    memeql(string.val, string.len, u8 ## slice, sizeof(slice) - 1)
#define CMP_TO_STRING(slice, string, off)   \
    memeql(u8 ## slice, sizeof(slice) - 1, string.val + off, MIN(string.len - off, sizeof(slice) - 1))

#define MAX_IDENT_LEN 120

struct C_Typed {
    u32     dst_begin;
    String  raw_alias;
};

struct C_Struct {
    String name;
    String data;
};

struct C_Enum {
    String name;
    String data;
};

struct C_Macro {
    String name;
    String args;
    String data;
};

struct TodoBadStrHs {
    u8 key[MAX_IDENT_LEN];
};

// TODO: use str_hm to
// add struct or define
struct Module {
    String *incs;
    String *funcs;
    C_Enum *enums;
    C_Typed *typed;
    C_Macro *defines;
    C_Struct *structs;
};

bool is_ident_start(u8 character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character == '_');
}

bool is_ident(u8 character) {
    return is_ident_start(character) || (character >= '0' && character <= '9');
}

bool read_ident(u32 *out_pos, String source) {
    if (*out_pos < source.len && is_ident_start(source.val[*out_pos])) {
        *out_pos += 1;
    }

    while (*out_pos < source.len && is_ident(source.val[*out_pos])) {
        *out_pos += 1;
    }

    return *out_pos < source.len;
}

bool skip_whitespace_and_new_line(u32 *out_pos, String source) {
    while (*out_pos < source.len 
        && (source.val[*out_pos] == ' ' || source.val[*out_pos] == '\n')) {
        *out_pos += 1;
    }

    return *out_pos < source.len;
}

void ignore_comments(String source, u32 *pos) {
    if          (*pos + 1 < source.len && source.val[*pos + 1] == '/') {
        *pos += 2;
        while (*pos < source.len && source.val[*pos] != '\n') {
            *pos += 1;
        }
    } else if   (*pos + 1 < source.len && source.val[*pos + 1] == '*') {
        *pos += 2;
        while (*pos + 1 < source.len && !(source.val[*pos] == '*' && source.val[*pos + 1] == '/')) {
            *pos += 1;
        }
        *pos += 1; // second one will get skipped by the parse loop
    }
}

void parse_c_enum(Arena *allocator, Module *out_mod, String source, u32 *pos) {
    *pos += sizeof("enum"); // '\0'
    assert(skip_whitespace_and_new_line(pos, source), S("expected enum ident but got eof"));

    u32 beg = *pos;
    assert(read_ident(pos, source), S("failed to read enum ident"));

    String name = str_copy(allocator, source, beg, *pos);
    assert(skip_whitespace_and_new_line(pos, source), S("expected struct body but got eof"));
    assert(source.val[*pos] == '{', S("expected opening brace after struct ident"));
    beg = *pos;
    *pos += 1;

    while (*pos < source.len && source.val[*pos] != '}') {
        *pos += 1;
    }
    *pos += 1;

    C_Enum cenum = {
        .name = name,
        .data = str_copy(allocator, source, beg, *pos),
    };
    array_push(allocator, out_mod->enums, cenum);
}

void parse_c_struct(Arena *allocator, Module *out_mod, String source, u32 *pos) {
    *pos += sizeof("struct"); // '\0'
    assert(skip_whitespace_and_new_line(pos, source), S("expected struct ident but got eof"));

    u32 beg = *pos;
    assert(read_ident(pos, source), S("failed to read struct ident"));

    String name = str_copy(allocator, source, beg, *pos);
    assert(skip_whitespace_and_new_line(pos, source), S("expected struct body but got eof"));
    assert(source.val[*pos] == '{', S("expected opening brace after struct ident"));
    *pos += 1;
    u16 braces = 1;
    beg = *pos;

    while (*pos < source.len && braces > 0) {
        if          (source.val[*pos] == '{') {
            braces += 1;
        } else if   (source.val[*pos] == '}') {
            braces -= 1;
        }

        *pos += 1;
    }

    C_Struct plex = {
        .name = name,
        .data = str_copy(allocator, source, beg, *pos - 1),
    };
    array_push(allocator, out_mod->structs, plex);
}

void parse_c_typedef(Arena *allocator, Module *out_mod, String source, u32 *pos) {
    u32 beg = *pos;
    *pos += sizeof("typedef");
    assert(skip_whitespace_and_new_line(pos, source), S("unexpected eof after typedef"));

    if (CMP_TO_STRING("struct ", source, *pos)) {
        *pos -= 1;
        return;
    }
    while (*pos < source.len && source.val[*pos] != ';') {
        *pos += 1;
    }
    *pos += 1;

    u32 last_space = 0;
    s32 prev = (*pos) - 1;
    while (prev > 0 && last_space == 0) {
        if (source.val[prev] == ' ' || source.val[prev] == '\t' || source.val[prev] == '\n') {
            last_space = prev;
        }
        prev -= 1;
    }

    assert(last_space != 0, S("expected destination name in typedef"));
    C_Typed c_typedef = {
        .raw_alias = str_copy(allocator, source, beg, *pos),
        .dst_begin = last_space,
    };
    array_push(allocator, out_mod->typed, c_typedef);
}

void parse_c_macros(Arena *allocator, Module *out_mod, String source, u32 *pos) {
    u32 beg = 0;

    if  (CMP_TO_STRING("#include ", source, *pos)) {
        beg = *pos;
        while (*pos < source.len && source.val[*pos] != '\n') {
            *pos += 1;
        }

        String include = str_copy(allocator, source, beg, *pos);
        array_push(allocator, out_mod->incs, include);
        return;
    } 

    if (!CMP_TO_STRING("#define ", source, *pos)) {
        return;
    }
    *pos += sizeof("#define");
    assert(skip_whitespace_and_new_line(pos, source), S("unexpected eof #define"));

    beg = *pos;
    assert(read_ident(pos, source), S("failed to read define macro identifer"));
    *pos += 1;

    String name = str_copy(allocator, source, beg, *pos);
    String args  = {0};
    assert(skip_whitespace_and_new_line(pos, source), S("expected define body after identifier"));

    if (*pos < source.len && source.val[*pos] == '(') {
        beg = *pos;
        u16 braces = 1;

        while (*pos < source.len && braces > 0) {
            if          (source.val[*pos] == '(') {
                braces += 1;
            } else if   (source.val[*pos] == ')') {
                braces -= 1;
            }
            
            *pos += 1;
        }

        args = str_copy(allocator, source, beg, *pos);
    }

    beg = *pos;
    u32 bks = 0;
    while (*pos < source.len && (source.val[*pos] != '\n' || bks > 0)) {
        if          (source.val[*pos] == '\n') {
            bks -= 1;
        } else if   (source.val[*pos] == '\\') {
            bks += 1;
        }
        *pos += 1;
    }

    C_Macro define = {
        .name = name,
        .args = args,
        .data = str_copy(allocator, source, beg, *pos),
    };
    array_push(allocator, out_mod->defines, define);
}

void parse_c_function(Arena *allocator, Module *out_mod, String source, u32 *pos) {
    u32 tmp = *pos + 1;
    u32 brc = 1;

    while (tmp < source.len && brc > 0) {
        if          (source.val[tmp] == '(') {
            brc += 1;
        } else if   (source.val[tmp] == ')') {
            brc -= 1;
        }

        tmp += 1;
    }

    brc = tmp;
    skip_whitespace_and_new_line(&tmp, source);
    if (tmp < source.len && source.val[tmp] == '{') {
        s64 i = *pos;
        s8 space_and_newline = 0;
    
        while (i > 0) {
            if (source.val[i] == ' ' || source.val[i] == '\n') {
                if (space_and_newline == 1) {
                    i += 1;
                    break;
                }
                space_and_newline += 1;
            }
            i -= 1;
        }

        u32 len = (brc - i) + 1;
        String header = {
            .val = alloc(allocator, u8, len),
            .len = len,
        };
        header.val[len - 1] = ';';
        for (u32 idx = i; idx < brc; idx++) {
            header.val[idx - i] = source.val[idx];
        }

        array_push(allocator, out_mod->funcs, header);
        brc = 1;
        *pos = tmp + 1;

        while (*pos < source.len && brc > 0) {
            if          (source.val[*pos] == '{') {
                brc += 1;
            } else if   (source.val[*pos] == '}') {
                brc -= 1;
            }

            *pos += 1;
        }
    }
}

void create_module_from_file(Arena *allocator, String file_name, Module *out_mod) {
    if (is_log_enabled) {
        printf("[INFO] reading file: `%s`\n", file_name.val);
    }

    u32 i = 0;
    String source = file_read_as_string_alloc(allocator, file_name);

    while (i < source.len) {
        switch (source.val[i]) {
            case 's':
                if (CMP_TO_STRING("struct ", source, i)) {
                    parse_c_struct(allocator, out_mod, source, &i);
                }
            break;
            case 'e':
                if (CMP_TO_STRING("enum ", source, i)) {
                    parse_c_enum(allocator, out_mod, source, &i);
                }
            case 't':
                if (CMP_TO_STRING("typedef ", source, i)) {
                    parse_c_typedef(allocator, out_mod, source, &i);
                }
            break;
            case '#':
                parse_c_macros(allocator, out_mod, source, &i);
            break;
            case '(':
                parse_c_function(allocator, out_mod, source, &i);
            break;
            case '/':
                ignore_comments(source, &i);
            break;
        }

        i += 1;
    }
}

void append_strings_with_nl(Arena *allocator, String *strs, StringBuilder **file) {
    for (u32 i = 0; i < array_len(strs); i++) {
        sb_push_str(allocator, *file, strs[i]);
        sb_push_char(allocator, *file, '\n');
    }
    sb_push_char(allocator, *file, '\n');
}

void append_c_typedefs_with_nl(Arena *allocator, Module *module, StringBuilder **file) {
    for (u32 i = 0; i < array_len(module->typed); i++) {
        sb_push_str(allocator, *file, module->typed[i].raw_alias);
        sb_push_char(allocator, *file, '\n');
    }
    for (u32 i = 0; i < array_len(module->structs); i++) {
        sb_push_str(allocator, *file, S("typedef struct "));
        sb_push_str(allocator, *file, module->structs[i].name);
        sb_push_char(allocator, *file, ' ');
        sb_push_str(allocator, *file, module->structs[i].name);
        sb_push_str(allocator, *file, S(";\n"));
    }
    for (u32 i = 0; i < array_len(module->enums); i++) {
        sb_push_str(allocator, *file, S("typedef enum "));
        sb_push_str(allocator, *file, module->enums[i].name);
        sb_push_char(allocator, *file, ' ');
        sb_push_str(allocator, *file, module->enums[i].name);
        sb_push_str(allocator, *file, S(";\n"));
    }

    sb_push_char(allocator, *file, '\n');
}

void append_c_macros_with_nl(Arena *allocator, C_Macro *macros, StringBuilder **file) {
    for (u32 i = 0; i < array_len(macros); i++) {
        sb_push_str(allocator, *file, S("#define "));
        sb_push_str(allocator, *file, macros[i].name);
        sb_push_str(allocator, *file, macros[i].args);
        sb_push_str(allocator, *file, macros[i].data);
        sb_push_char(allocator, *file, '\n');
    }
    sb_push_char(allocator, *file, '\n');
}

bool resolve_type(
        Arena *allocator, 
        TodoBadStrHs *is_alias,
        TodoBadStrHs **visited, 
        Module *module, 
        StringBuilder **output,
        u32 index
) {
    // struct A {
    //      Field b;
    // }
    //
    // struct Field {
    //      Area a;
    //      u32 *cap;
    // }

    TodoBadStrHs value = {0};
    C_Struct plex = module->structs[index];
    assert(plex.name.len < MAX_IDENT_LEN, S("struct identifier is larger than max allowed len"));
    memcpy(value.key, plex.name.val, plex.name.len);

    // NOTE: not checking aliases
    if (hm_get(is_alias, value.key) != NULL) {
        return true;
    }
    // NOTE: checking for cycles
    if (hm_get(*visited, value.key) != NULL) {
        printf("ERROR: ");
        for (u32 i = 0; i < plex.name.len; i++) {
            printf("%c", plex.name.val[i]);
        }
        return false;
    }
    hm_put(allocator, *visited, value.key, value);

    u32 field_pos = 0;

    while (field_pos < plex.data.len) {
        skip_whitespace_and_new_line(&field_pos, plex.data);
        ignore_comments(plex.data, &field_pos);
        skip_whitespace_and_new_line(&field_pos, plex.data);
 
        u32 ident_start = field_pos;
        read_ident(&field_pos, plex.data);

        u32 ident_end = field_pos;
        u32 ident_len = ident_end - ident_start;

        if (ident_len == 0) {
            assert(field_pos == plex.data.len, S("no identifier found but len is not the same"));
            continue;
        }

        assert(ident_len < MAX_IDENT_LEN, S("type of field is longer than max ident len"));
        TodoBadStrHs ident = {0};
        for (u32 i = 0; i < ident_len; i++) {
            ident.key[i] = plex.data.val[ident_start + i];
        }

        skip_whitespace_and_new_line(&field_pos, module->structs[index].data);
        if ((field_pos < module->structs[index].data.len 
            && module->structs[index].data.val[field_pos] == '*'
            )
            || hm_get(is_alias, ident.key) != NULL
        ) {
            while (field_pos < plex.data.len && plex.data.val[field_pos] != ';') {
                field_pos += 1;
            }
            field_pos += 1;
            continue;
        }

        for (u32 i = 0; i < array_len(module->structs); i++) {
            C_Struct *cur = &module->structs[i];
            TodoBadStrHs name = {0};
            memcpy(name.key, cur->name.val, cur->name.len);

            if (memeql(name.key, cur->name.len, plex.data.val + ident_start, ident_end - ident_start)
            && hm_get(*visited, name.key) == NULL
            && !resolve_type(allocator, is_alias, visited, module, output, i)) {
                    return false;
            }
        }

        while (field_pos < plex.data.len && plex.data.val[field_pos] != ';') {
            field_pos += 1;
        }
        field_pos += 1;
    }


    sb_push_str(allocator, *output, S("struct "));
    sb_push_str(allocator, *output, plex.name);
    sb_push_char(allocator, *output, '{');
    sb_push_str(allocator, *output, plex.data);
    sb_push_str(allocator, *output, S("};\n"));
    return true;
}

void create_context(Arena *allocator, StringBuilder **output) {
    const String ctx = S(
        "typedef struct Context {\n"
        "   u8 **environment_vars;\n"
        "   // TODO: maybe put allocator here too \n"
        "} Context;\n"
        "Context ctx = {0};\n"
        "\n"
        "void init_ctx(u8 **environment_vars) {\n"
        "   ctx.environment_vars = environment_vars;\n"
        "}\n"
        "\n"
    );
    sb_push_str(allocator, *output, ctx);
}

s32 main(s32 argc, char **argv, char **environ) {
    if (argc == 1) {
        printf("mob: no input files\n");
        return 0;
    }

    init_ctx((u8**)environ); 
    Arena allocator = {0};
    arena_init(&allocator, 2 << 24);

    String *args = NULL;
    String log_flag = S("--enable_log");
    u32 skip_first_arg = 0;

    for (u32 i = 1; i < argc; i++) {
        array_push(&allocator, args, from_c_string(argv[i]));
    }

    if (memeql(log_flag.val, log_flag.len, args[0].val, args[0].len)) {
        is_log_enabled = true;
        skip_first_arg = 1;
    }

    TodoBadStrHs *visited = NULL;
    TodoBadStrHs *is_alias = NULL;
    TodoBadStrHs empty = {0};
    u8 key[MAX_IDENT_LEN]; 
    for (u32 i = 0; i < MAX_IDENT_LEN; i++) key[i] = 0;
    hm_put(&allocator, visited, key, empty);

    Module module = {0};
    StringBuilder *unit = NULL;
    sb_push_str(&allocator, unit, S("// GENERATED BY MOB\n// ================\n\n"));

    for (u32 i = skip_first_arg; i < array_len(args); i++) {
        create_module_from_file(&allocator, args[i], &module);
    }

    append_strings_with_nl(&allocator, module.incs, &unit);
    append_c_typedefs_with_nl(&allocator, &module, &unit);
    append_c_macros_with_nl(&allocator, module.defines, &unit);

    for (u32 i = 0; i < array_len(module.typed); i++) {
        s32 typed_pos = module.typed[i].raw_alias.len - 2;

        while (typed_pos > 0 && !is_ident(module.typed[i].raw_alias.val[typed_pos])) {
            typed_pos -= 1;
        }
        s32 end = typed_pos + 1;

        while (typed_pos > 0 && is_ident(module.typed[i].raw_alias.val[typed_pos])) {
            typed_pos -= 1;
        }

        TodoBadStrHs kv = {0};
        memcpy(kv.key, module.typed[i].raw_alias.val + (typed_pos + 1), end - (typed_pos + 1));
        hm_put(&allocator, is_alias, kv.key, empty);
    }

    // NOT the most optimal solution since I could 
    // not make a hashmap for the structs but well
    for (u32 i = 0; i < array_len(module.structs); i++) {
        TodoBadStrHs kv = {0};
        memcpy(kv.key, module.structs[i].name.val, module.structs[i].name.len);

        // NOTE: already set
        // in recursive call
        if (hm_get(visited, kv.key) != NULL) {
            continue;
        }

        // NOTE: cycle detected
        if (!resolve_type(&allocator, is_alias, &visited, &module, &unit, i)) {
            printf(" is a cyclic dependency of `");
            for (u32 j = 0; j < module.structs[i].name.len; j++) {
                printf("%c", module.structs[i].name.val[j]);
            }
            printf("`\n");
        }
    }

    sb_push_char(&allocator, unit, '\n');
    append_strings_with_nl(&allocator, module.funcs, &unit);
    create_context(&allocator, &unit);

    for (u32 i = skip_first_arg; i < array_len(args); i++) {
        sb_push_str(&allocator, unit, S("#include \""));
        sb_push_str(&allocator, unit, args[i]);
        sb_push_str(&allocator, unit, S("\"\n"));
    }
    sb_push_char(&allocator, unit, '\n');

    String content = {
        .val = unit,
        .len = array_len(unit),
    };
    assert(write_string_to_file(content, BUILD), S("failed to write output file"));

    arena_deinit(&allocator);
    return 0;
}

