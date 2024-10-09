#ifndef U_VECTOR_29
#define U_VECTOR_29

#include <stddef.h>

#include "u_vector_def.h"

UVector u_vector_alloc(size_t elementSize, size_t size);

void u_vector_free(UVector *pVector);

int u_vector_scale(UVector *pVector, size_t size);

#define U_VECTOR_DECLARE(element, size) u_vector_alloc(sizeof(element), size)

#endif // U_VECTOR_29
