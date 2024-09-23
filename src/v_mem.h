#ifndef V_MEMORY_29
#define V_MEMORY_29

#include "raymath.h"
#include "SDL_stdinc.h"
#include <vulkan/vulkan.h>

typedef struct {
    Vector2 pos;
    Vector3 color;
} Vertex;

extern const Vertex vertices[6];

extern const VkVertexInputBindingDescription vertexBindingDescription;
extern const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2];

int v_alloc_vertex_buffer();
Uint32 v_find_memory_type(Uint32 typeFilter, VkMemoryPropertyFlags properties);

#endif // V_MEMORY_29
