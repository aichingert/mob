
struct String {
    u8 *val;
    u64 len;
};

struct Strings {
    __ARRAY_HEADER__;
    String *arr;
};

#define S(value) ((String){                             \
        .val = u8 ## value,                             \
        .len = (sizeof(value) / sizeof(value[0]) - 1)   \
        })                                              \

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

bool c_string_begins_with(const char *c_s, String s) {
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

String str_concat(Arena *arena, Strings strs) {
    u64 len = 0;
    u64 pos = 0;

    for (u64 i = 0; i < strs.len; i++) {
        len += strs.arr[i].len;
    }

    u8 *mem = alloc(arena, u8, len);

    for (u64 i = 0; i < strs.len; i++) {
        for (u64 j = 0; j < strs.arr[i].len; j++) {
            mem[j + pos] = strs.arr[i].val[j];
        }

        pos += strs.arr[i].len;
    }

    return (String){
        .val = mem,
        .len = len,
    };
}
