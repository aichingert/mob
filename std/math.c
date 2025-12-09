#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define ABS(a)      ((a) > ((s64)0) ? (a) : ((-(a))))

f64 fabs(f64 a) {
    struct MOB_FLOAT_U64 {
        union {
            u64 uval;
            f64 fval;
        };
    } value;

    value.fval = a;
    value.uval &= 0x8000000000000000u - 1u;
    return value.fval;
}

u64 pow(u64 base, u16 exponent) {
    if (exponent == 0) {
        return 1;
    }

    u64 ans = base;

    // TODO: improve since pow(8, 8) -> could be 
    // 8 * 8        -> 64
    // 64 * 64      -> 4096
    // 4096 * 4096  -> 16777216 = 8 * 8 * 8 * 8....
    for (u16 i = 1; i < exponent; i++) {
        ans *= base;
    }

    return ans;
}

f64 hsqrt(u64 num) {
    f64 fnumber = num;
    f64 initial = fnumber / 2.0;
    f64 epsilon = 1e-5;

    while (fabs(fnumber - initial * initial) >= epsilon) {
        initial = (initial + fnumber / initial) / 2.0;
    }

    return initial;
}
