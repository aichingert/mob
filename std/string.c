struct String {
    char *val;
    u64 len;
};

#define S(value) ((String){                             \
        .val = value,                                   \
        .len = (sizeof(value) / sizeof(value[0]) - 1)   \
        })                                              \

String from_c_string(char *str) {
    String s = {0};

    if (str == NULL) {
        return s;
    }

    s.val = str;

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


