static_assert(sizeof(char)                      == 1);
static_assert(sizeof(short)                     == 2);
static_assert(sizeof(int)                       == 4);
static_assert(sizeof(long long int)             == 8);
static_assert(sizeof(unsigned long long int)    == 8);
static_assert(sizeof(void*)                     == sizeof(unsigned long long int));
static_assert(sizeof(float)                     == 4);
static_assert(sizeof(double)                    == 8);

typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;

typedef char                    s8;
typedef short                   s16;
typedef int                     s32;
typedef long long int           s64;

typedef float                   f32;
typedef double                  f64;

#define NULL                    ((void *)0)
