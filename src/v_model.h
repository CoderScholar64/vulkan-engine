#ifndef V_MODEL_29
#define V_MODEL_29

#include "context.h"
#include "v_buffer.h"
#include "v_model_def.h"

VEngineResult v_model_load(Context *this, const char *const pUTF8Filepath, unsigned *pModelAmount, VModelData **ppVModelData);

void v_model_draw_record(Context *this, VkCommandBuffer commandBuffer, VModelData *pModelData, unsigned numInstances, VBufferPushConstantObject *pPushConstantObjects);

#endif // V_MODEL_29
