#ifndef V_MODEL_29
#define V_MODEL_29

#include "v_buffer.h"

typedef struct VModelData {
    uint32_t vertexAmount;
    VkDeviceSize vertexOffset;
    VkIndexType indexType;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
} VModelData;

VEngineResult v_load_model(const char *const pUTF8Filepath, unsigned *pModelAmount, VModelData **ppVModelData);

void v_record_model_draw(VkCommandBuffer commandBuffer, VModelData *pModelData, unsigned numInstances, PushConstantObject *pPushConstantObjects);

#endif // V_MODEL_29
