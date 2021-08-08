#ifndef STB_STRETCHY_BUFFER_H_INCLUDED
#define STB_STRETCHY_BUFFER_H_INCLUDED

#define sb_free(a)         ((a) ? free(stb__sbraw(a)),0 : 0)
#define sb_push(a, v)       (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define sb_count(a)        ((a) ? stb__sbn(a) : 0)
#define sb_add(a, n)        (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define sb_last(a)         ((a)[stb__sbn(a)-1])

#define stb__sbraw(a) ((int *) (void *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a, n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a, n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a, n)      (*((void **)&(a)) = stb__sbgrowf((a), (n), sizeof(*(a))))

#include <stdlib.h>

static void *stb__sbgrowf(void *arr, int increment, int itemsize) {
    int dbl_cur = arr ? 2 * stb__sbm(arr) : 0;
    int min_needed = sb_count(arr) + increment;
    int m = dbl_cur > min_needed ? dbl_cur : min_needed;
    int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int) * 2);
    if (p) {
        if (!arr)
            p[1] = 0;
        p[0] = m;
        return p + 2;
    } else {
#ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
        STRETCHY_BUFFER_OUT_OF_MEMORY ;
#endif
        return (void *) (2 * sizeof(int)); // try to force a NULL pointer exception later
    }
}

#endif
