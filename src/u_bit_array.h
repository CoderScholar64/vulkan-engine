#ifndef U_BIT_ARRAY_29
#define U_BIT_ARRAY_29

typedef unsigned UBitElement;

#define U_BITS_PER_BYTE 8

#define U_BIT_ELEMENT_BIT_COUNT (sizeof(UBitElement) * U_BITS_PER_BYTE)

#define U_BIT_ARRAY_INDEX_AMOUNT(arrayElementAmount) (arrayElementAmount / U_BIT_ELEMENT_BIT_COUNT + (0 != (arrayElementAmount % U_BIT_ELEMENT_BIT_COUNT)))

#define U_BIT_ARRAY_SIZE(arrayElementAmount) (sizeof(UBitElement) * U_BIT_ARRAY_INDEX_AMOUNT(arrayElementAmount))

#define U_BIT_ARRAY_GET(array, index) ((array[index / U_BIT_ELEMENT_BIT_COUNT] >> (index % U_BIT_ELEMENT_BIT_COUNT)) & 1)

#define U_BIT_ARRAY_SET(array, index, bitValue)\
    array[index / U_BIT_ELEMENT_BIT_COUNT] &= (~((UBitElement)0)) ^ (1 << (index % U_BIT_ELEMENT_BIT_COUNT));\
    if(bitValue == 1)\
        array[index / U_BIT_ELEMENT_BIT_COUNT] |= (1 << (index % U_BIT_ELEMENT_BIT_COUNT))


#endif // U_BIT_ARRAY_29
