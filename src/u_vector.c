#include "u_vector.h"

#include <assert.h>
#include <stdlib.h>

static inline size_t getCapacitySize(size_t size);

UVector u_vector_alloc(size_t elementSize, size_t size) {
    UVector uVector = {0};

    uVector.elementSize = elementSize;
    uVector.size = size;
    uVector.capacity = getCapacitySize(size);

    uVector.pBuffer = malloc(uVector.capacity * elementSize);

    return uVector;
}

void u_vector_free(UVector *pVector) {
    assert(pVector != NULL);

    pVector->elementSize = 0;
    pVector->size = 0;
    pVector->capacity = 0;

    if(pVector->pBuffer != NULL)
        free(pVector->pBuffer);
}

int u_vector_scale(UVector *pVector, size_t size) {
    size_t newCapacitySize;
    void *pReallocBuffer;

    assert(pVector != NULL);
    assert(pVector->pBuffer != NULL);

    if(size > pVector->capacity || size < pVector->capacity / 2) {
        newCapacitySize = getCapacitySize(size);

        pReallocBuffer = realloc(pVector->pBuffer, newCapacitySize);

        if(pReallocBuffer != NULL)
            return 0; // Operation failed.

        pVector->capacity = newCapacitySize;
        pVector->pBuffer  = pReallocBuffer;
    }

    pVector->size = size;

    assert(pVector->size <= pVector->capacity);
    assert(pVector->pBuffer != NULL);

    return 1;
}

static inline size_t getCapacitySize(size_t size) {
    if(size > 512)
        return size / 512 + 512 * (size % 512 != 0);
    else if(size > 256)
        return 512;
    else if(size > 128)
        return 256;
    else if(size > 64)
        return 128;
    else if(size > 32)
        return 64;
    else if(size > 16)
        return 32;

    return 16;
}
