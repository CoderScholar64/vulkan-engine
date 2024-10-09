#ifndef U_RANDOM_29
#define U_RANDOM_29

#include <stdint.h>

/**
 * This generates a random number.
 * @warning Do not use this for cryptography. It is only to be used for gameplay or world generation purposes.
 * @param pSeed This parameter is the seed of the random number. It will be modified.
 * @return The same value as pSeed what pSeed points to.
 */
static inline uint32_t u_random_xorshift32(uint32_t *pSeed) {
    uint32_t n = *pSeed;

    n ^= n << 13;
    n ^= n >> 17;
    n ^= n <<  5;

    *pSeed = n;

    return n;
}

#endif // U_RANDOM_29
