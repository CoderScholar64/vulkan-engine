#ifndef V_BUFFER_DEFINE_29
#define V_BUFFER_DEFINE_29

#include "v_raymath.h"

typedef struct {
    Vector3 pos;
    Vector3 color;
    Vector2 texCoord;
} VBufferVertex;

typedef struct {
    Vector4 color;
} VBufferUniformBufferObject;

typedef struct {
    Matrix matrix;
} VBufferPushConstantObject;

extern const VkVertexInputBindingDescription vBufferVertexBindingDescription;
extern const VkVertexInputAttributeDescription vBufferVertexInputAttributeDescriptions[3];

#endif // V_BUFFER_DEFINE_29
