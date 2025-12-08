// NOTE: merge_sort for the dynamic array implementation provided by mob

//#define merge_sort(mob_array_to_sort, sort_ascending, mob_internal_cmp)     \
//
//
//
//void merge_sort(void *array, u64 size_of_t, bool ascending, s8 (*cmp)(void *a, void *b)) {
//
//}

#define bubble_sort_impl(T)                                     \
    void bubble_sort_ ## T (                                    \
            T *mob_internal_array,                              \
            bool ascending,                                     \
            s8 (*cmp)(T *a, T *b)) {                            \
        for (u64 i = 0; i  < array_len(mob_internal_array); i++) {  \
            printf("%u/%u\n", i, array_len(mob_internal_array));    \
            for (u64 j = i + 1; j < array_len(mob_internal_array); j++) {   \
                s8 result = cmp(&mob_internal_array[i], &mob_internal_array[j]);    \
                if (!ascending) {                                                   \
                    result *= -1;                                                   \
                }                                                                   \
                                                                                    \
                if (result > 0) {                                                   \
                    u8 value[sizeof(*mob_internal_array)];                          \
                    memcpy(value, (u8*)&mob_internal_array[i], sizeof(*mob_internal_array)); \
                    memcpy((u8*)&mob_internal_array[i], (u8*)&mob_internal_array[j], sizeof(*mob_internal_array));    \
                    memcpy((u8*)&mob_internal_array[j], value, sizeof(*mob_internal_array)); \
                }   \
            }   \
        }   \
    }


// TODO: think about having a macro that implements merge sort or a generic merge sort implementation
//#define merge_sort_impl(array_to_sort, T)
//    void merge_sort_ ## T (T *mob_internal_array, s8 (*mob_internal_cmp)(void *a, void *b), bool sort_ascending) {
//        u64 len = array_len(mob_internal_array);
//
//
//        merge_t(mob_internal_array, sizeof(T), len, mob_internal_cmp, sort_ascending)
//    }
//    
//void merge_t(
//        void *array, 
//        u64 size_of_t, u64 array_len, s8 (*cmp)(void *a, void *b), bool sort_ascending) {
//
//}
//
// x1 x5 x9 x2 x9 x9
//
// x1 x5 x9 | x2 x9 x9
//
// x1 x5 | x9 | x2 x9 | x9
// cmp(x1 x5) | x9 | cmp(x2 x9) | x9
// x1 x5 x9       | x2 x9 x9
// x1 x2 x5 x9 x9 x9
