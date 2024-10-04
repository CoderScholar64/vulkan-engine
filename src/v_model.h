#ifndef V_MODEL_29
#define V_MODEL_29

#include "v_buffer.h"

typedef struct VModelData {
    unsigned vertexAmount;
    VkDeviceSize vertexOffset;
    VkIndexType indexType;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
} VModelData;

VEngineResult v_load_model(const char *const pUTF8Filepath, unsigned *pModelAmount, VModelData **ppVModelData);

#endif // V_MODEL_29
