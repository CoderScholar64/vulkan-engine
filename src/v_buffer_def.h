#ifndef V_BUFFER_DEFINE_29
#define V_BUFFER_DEFINE_29

#include "v_raymath.h"

typedef struct {
    Vector3 pos;
    Vector3 color;
    Vector2 texCoord;
} Vertex;

typedef struct {
    Vector4 color;
} UniformBufferObject;

typedef struct {
    Matrix matrix;
} PushConstantObject;

extern const VkVertexInputBindingDescription vertexBindingDescription;
extern const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[3];

#endif // V_BUFFER_DEFINE_29
