#ifndef V_MODEL_DEF_29
#define V_MODEL_DEF_29

#include <stddef.h>
#include <vulkan/vulkan.h>

#include "u_vector_def.h"
#include "v_buffer_def.h"

typedef struct VModelData {
    char name[32];
    uint32_t vertexAmount;
    VkDeviceSize vertexOffset;
    VkIndexType indexType;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
} VModelData;


typedef struct VModelArray {
    VModelData *pModelData; // Reference do not delete.
    UVector instanceVector; // VBufferPushConstantObject
} VModelArray;

#endif // V_MODEL_DEF_29
