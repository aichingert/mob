#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define ABS(a)      ((a) > ((s64)0) ? (a) : (-a))

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
