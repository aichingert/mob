struct String {
    u8 *val;
    u64 len;
};

typedef u8 StringBuilder;

#define S(value) ((String){                                     \
        .val = u8 ## value,                                     \
        .len = (sizeof(value) / sizeof(value[0]) - 1)           \
        })

// TODO: make this better
#define mob_to_string_impl(type)                                \
    String to_string_ ## type(Arena *allocator, type number) {  \
        u64 rev = 0;                                            \
        u64 len = 0;                                            \
        bool sign = number < 0;                                 \
        u64 num = ABS(number);                                  \
                                                                \
        while (num > 0) {                                       \
            rev = rev * 10 + num % 10;                          \
            num /= 10;                                          \
            len += 1;                                           \
        }                                                       \
                                                                \
        if (sign) {                                             \
            len += 1;                                           \
        }                                                       \
                                                                \
        String str = {                                          \
            .val = alloc(allocator, u8, len),                   \
            .len = len,                                         \
        };                                                      \
                                                                \
        len = 0;                                                \
        if (sign) {                                             \
            str.val[len] = '-';                                 \
            len += 1;                                           \
        }                                                       \
        while (rev > 0) {                                       \
            str.val[len] = '0' + (rev % 10);                    \
            rev /= 10;                                          \
            len += 1;                                           \
        }                                                       \
        while (len < str.len) {                                 \
            str.val[len] = '0';                                 \
            len += 1;                                           \
        }                                                       \
                                                                \
        return str;                                             \
    }                                                           

// TODO: probably wrong 
// for edge cases
mob_to_string_impl(u8);
mob_to_string_impl(u16);
mob_to_string_impl(u32);
mob_to_string_impl(u64);
mob_to_string_impl(s8);
mob_to_string_impl(s16);
mob_to_string_impl(s32);
mob_to_string_impl(s64);

// FIXME: if this stds error handling 
// gets better this should be changed
// NOTE: exlusive end
String str_copy(Arena *allocator, String str, u64 beg, u64 end) {
    printf("%lu | %lu | %lu\n", str.len, beg, end);
    assert(str.len >= end && beg <= end, S("range is not available in string"));

    u64 len = end - beg;
    u8 *mem = alloc(allocator, u8, len);

    for (u64 i = beg; i < end; i++) {
        mem[i - beg] = str.val[i];
    }

    return (String){ .val = mem, .len = len };
}

String from_c_string(char *str) {
    String s = {0};

    if (str == NULL) {
        return s;
    }

    s.val = (u8*)str;

    while (str[s.len] != '\0') {
        s.len += 1;
    }

    return s;
}

bool c_string_begins_with_str(const char *c_s, String s) {
    if (s.val == NULL && c_s == NULL) {
        return true;
    }
    if (s.val == NULL || c_s == NULL) {
        return false;
    }

    for (u64 i = 0; i < s.len; i++) {
        if (c_s[i] == '\0' || c_s[i] != s.val[i]) {
            return false;
        }
    }

    return true;
}

bool str_begins_with(String a, String b) {
    if (b.len > a.len) {
        return false;
    }

    for (u64 i = 0; i < b.len; i++) {
        if (a.val[i] == b.val[i]) {
            return true;
        }
    }

    return false;
}

String str_concat(Arena *arena, String *strs) {
    u64 len = 0;
    u64 pos = 0;

    for (u64 i = 0; i < array_len(strs); i++) {
        len += strs[i].len;
    }

    u8 *mem = alloc(arena, u8, len);

    for (u64 i = 0; i < array_len(strs); i++) {
        for (u64 j = 0; j < strs[i].len; j++) {
            mem[j + pos] = strs[i].val[j];
        }

        pos += strs[i].len;
    }

    return (String){
        .val = mem,
        .len = len,
    };
}


// STRING_BUILDER

#define sb_push_char(arena, sb, c)  array_push(arena, sb, (u8)c)
#define sb_push_str(arena, sb, str) (array_grow((arena), (sb), (str).len), mob_sb_push_str((sb), (str)))

void mob_sb_push_str(StringBuilder *sb, String str) {
    assert(array_cap(sb) >= array_len(sb) + str.len, S("string builder does not have enough cap"));

    memcpy(sb + array_len(sb), str.val, str.len);
    array_header(sb)->len += str.len;
}

