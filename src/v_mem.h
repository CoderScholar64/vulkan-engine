#ifndef V_MEMORY_29
#define V_MEMORY_29

#include "v_results.h"

#include "raymath.h"
#include "SDL_stdinc.h"
#include <vulkan/vulkan.h>

typedef struct {
    Vector2 pos;
    Vector3 color;
} Vertex;

extern const Vertex builtin_vertices[6];

extern const VkVertexInputBindingDescription vertexBindingDescription;
extern const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2];

VEngineResult v_alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory);

/**
 * This function allocates a built-in vertex buffer to the context.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_builtin_vertex_buffer();

/**
 * This function copies the srcBuffer to dstBuffer.
 * @warning Make sure that v_init() is called first. Also srcBuffer must have VK_BUFFER_USAGE_TRANSFER_SRC_BIT, and the dstBuffer VK_BUFFER_USAGE_TRANSFER_DST_BIT.
 * @param srcBuffer the Vulkan source buffer.
 * @param srcOffset the Vulkan source offset.
 * @param dstBuffer the Vulkan destination buffer.
 * @param dstOffset the Vulkan destination offset.
 * @param size the size in bytes of BOTH buffers.
 * @return A VEngineResult. If its type is VE_SUCCESS then srcBuffer is successfully copied to dstBuffer. If VE_COPY_BUFFER_FAILURE then the copying failed.
 */
VEngineResult v_copy_buffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size);

/**
 * Find the memory buffer from
 * @warning Make sure that v_init() is called first.
 * @note If you get a non-zero return you would need to subtract it by one to get the actual index.
 * @param typeFilter The memory type bits as needed from the VkMemoryRequirements::memoryTypeBits
 * @param properties The desired property flags of the memory index.
 * @return If zero then this function failed to find the memory type with the properties and typeFilter specificed.
 */
Uint32 v_find_memory_type_index(Uint32 typeFilter, VkMemoryPropertyFlags properties);

#endif // V_MEMORY_29
