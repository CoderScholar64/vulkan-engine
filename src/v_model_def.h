#ifndef V_MODEL_DEF_29
#define V_MODEL_DEF_29

typedef struct VModelData {
    uint32_t vertexAmount;
    VkDeviceSize vertexOffset;
    VkIndexType indexType;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
} VModelData;

#endif // V_MODEL_DEF_29
