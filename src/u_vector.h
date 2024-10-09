#ifndef U_VECTOR_29
#define U_VECTOR_29

#include <stddef.h>

#include "u_vector_def.h"

/**
 * @warning Check UVector->pBuffer to see if the allocation was successful.
 * @param elementSize The element size in bytes that needs to be allocated per element.
 * @param size The number of elements for the initial array.
 * @return A complete UVector.
 */
UVector u_vector_alloc(size_t elementSize, size_t size);

/**
 * @warning This only deletes pVector->pBuffer and not the struct itself.
 * @param pVector The vector to become free.
 */
void u_vector_free(UVector *pVector);

int u_vector_scale(UVector *pVector, size_t size);

/**
 * Declare a UVector.
 * @note This is so the programmer would not have to put sizeof everywhere.
 * @warning Check UVector->pBuffer to see if the allocation was successful.
 * @param element The element that needs to be allocated.
 * @param size The number of elements for the initial array.
 * @return A UVector containing numerious elements.
 */
#define U_VECTOR_DECLARE(element, size) u_vector_alloc(sizeof(element), size)

#endif // U_VECTOR_29
