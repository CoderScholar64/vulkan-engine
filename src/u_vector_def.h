#ifndef U_VECTOR_DEF_29
#define U_VECTOR_DEF_29

#include <stddef.h>

typedef struct UVector {
    size_t elementSize;
    size_t size;
    size_t capacity;

    void *pBuffer;
} UVector;

#endif // U_VECTOR_DEF_29
