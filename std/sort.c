// NOTE: merge_sort for the dynamic array implementation provided by mob

#define sort (array, T, cmp) ((void*)0)

// TODO: think about having a macro that implements merge sort or a generic merge sort implementation
//#define merge_sort_impl(array_to_sort, T)
//    void merge_sort_ ## T (T *mob_internal_array, s8 (*mob_internal_cmp)(void *a, void *b), bool sort_ascending) {
//        u64 len = array_len(mob_internal_array);
//
//
//        merge_t(mob_internal_array, sizeof(T), len, mob_internal_cmp, sort_ascending)
//    }
//    
//void merge_t(void *array, u64 size_of_t, u64 array_len, s8 (*cmp)(void *a, void *b), bool sort_ascending) {
//    if          (array_len == 1) {
//        return;
//    } else if   (array_len == 2) {
//        s8 result = cmp(array, array + size_of_t);
//
//        if ((result == 1 && sort_ascending) || (result == -1 && !sort_ascending)) {
//            u8 tmp[size_of_t];
//            memcpy(tmp, array, size_of_t);
//            memcpy(array, array + size_of_t, size_of_t);
//            memcpy(array + size_of_t, tmp, size_of_t);
//        }
//
//        return;
//    }
//
//    u64 mid = array_len / 2;
//    merge_t(array, size_of_t, mid, 
//
//}

// x1 x5 x9 x2 x9 x9
//
// x1 x5 x9 | x2 x9 x9
//
// x1 x5 | x9 | x2 x9 | x9
// cmp(x1 x5) | x9 | cmp(x2 x9) | x9
// x1 x5 x9       | x2 x9 x9
// x1 x2 x5 x9 x9 x9
