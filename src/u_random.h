#ifndef U_RANDOM_29
#define U_RANDOM_29

#include <stdint.h>

static inline uint32_t u_random_xorshift32(uint32_t *pSeed) {
    uint32_t n = *pSeed;

    n ^= n << 13;
    n ^= n >> 17;
    n ^= n <<  5;

    *pSeed = n;

    return n;
}

#endif // U_RANDOM_29
