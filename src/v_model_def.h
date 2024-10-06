#ifndef V_MODEL_DEF_29
#define V_MODEL_DEF_29

#include <stddef.h>
#include <vulkan/vulkan.h>

typedef struct VModelData {
    char name[64];
    uint32_t vertexAmount;
    VkDeviceSize vertexOffset;
    VkIndexType indexType;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
} VModelData;

#endif // V_MODEL_DEF_29
