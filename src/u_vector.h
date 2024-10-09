#ifndef U_VECTOR_29
#define U_VECTOR_29

#include <stddef.h>

typedef struct UVector {
    size_t elementSize;
    size_t size;
    size_t capacity;

    void *pBuffer;
} UVector;

UVector v_vector_alloc(size_t elementSize, size_t size);

void v_vector_free(UVector *pVector);

void v_vector_scale(UVector *pVector, size_t size);

#define V_VECTOR_DECLARE(element, size) v_vector_alloc(sizeof(element), size)

#endif // U_VECTOR_29
