#ifndef V_MODEL_DEF_29
#define V_MODEL_DEF_29

#include <stddef.h>
#include <vulkan/vulkan.h>

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
    size_t instanceAmount;
    PushConstantObject instances[];
} VModelArray;

#endif // V_MODEL_DEF_29
