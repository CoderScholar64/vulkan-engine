#ifndef V_MODEL_29
#define V_MODEL_29

#include "context.h"
#include "v_buffer.h"
#include "v_model_def.h"

VEngineResult v_load_models(Context *this, const char *const pUTF8Filepath, unsigned *pModelAmount, VModelData **ppVModelData);

void v_record_model_draws(Context *this, VkCommandBuffer commandBuffer, VModelData *pModelData, unsigned numInstances, PushConstantObject *pPushConstantObjects);

#endif // V_MODEL_29
