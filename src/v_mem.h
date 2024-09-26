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

typedef struct {
    Matrix model;
    Matrix view;
    Matrix proj;
} UniformBufferObject;


extern const Vertex builtin_vertices[4];
extern const Uint16 builtin_indexes[6];

extern const VkVertexInputBindingDescription vertexBindingDescription;
extern const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2];

/**
 * Allocate a singular buffer.
 * @note This is for more advanced usage for a general purpose one use v_alloc_static_buffer().
 * @warning Make sure that v_init() is called first.
 * @param size the Vulkan buffer to allocate.
 * @param usageFlags the VkBufferUsageFlags for the newely allocated buffer.
 * @param properties The desired property flags of the memory index.
 * @param pBuffer An unallocated reference to a vulkan buffer. @warning Make sure that pBuffer is unallocated before hand.
 * @param pBufferMemory An unallocated reference to a vulkan memory buffer. @warning Make sure that pBufferMemory is unallocated before hand.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory);

/**
 * Allocate a static buffer.
 * @warning Make sure that v_init() is called first.
 * @param pData the source of the data.
 * @param sizeOfData the size in bytes of the source.
 * @param pBuffer An unallocated reference to a vulkan buffer. @warning Make sure that pBuffer is unallocated before hand.
 * @param pBufferMemory An unallocated reference to a vulkan memory buffer. @warning Make sure that pBufferMemory is unallocated before hand.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created
 */
VEngineResult v_alloc_static_buffer(const void *pData, size_t sizeOfData, VkBuffer *pBuffer, VkBufferUsageFlags usageFlags, VkDeviceMemory *pBufferMemory);

/**
 * This function allocates a built-in vertex buffer to the context.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_builtin_vertex_buffer();

/**
 * This function allocates a built-in index buffer to the context.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_builtin_index_buffer();

/**
 * This function allocates a built-in uniform buffer to the context.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_builtin_uniform_buffers();

VEngineResult v_alloc_image(Uint32 width, Uint32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory);

/**
 * This function copies the srcBuffer to dstBuffer via vulkan.
 * @warning Make sure that v_init() is called first. Also srcBuffer must have VK_BUFFER_USAGE_TRANSFER_SRC_BIT, and the dstBuffer VK_BUFFER_USAGE_TRANSFER_DST_BIT.
 * @param srcBuffer the Vulkan source buffer.
 * @param srcOffset the Vulkan source offset.
 * @param dstBuffer the Vulkan destination buffer.
 * @param dstOffset the Vulkan destination offset.
 * @param size the size in bytes of BOTH buffers.
 * @return A VEngineResult. If its type is VE_SUCCESS then srcBuffer is successfully copied to dstBuffer. If VE_COPY_BUFFER_FAILURE then the copying failed.
 */
VEngineResult v_copy_buffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size);

VEngineResult v_begin_one_time_command_buffer(VkCommandBuffer *pCommandBuffer);
VEngineResult   v_end_one_time_command_buffer(VkCommandBuffer *pCommandBuffer);

VEngineResult v_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
VEngineResult v_copy_buffer_to_image(VkBuffer buffer, VkImage image, Uint32 width, Uint32 height);

VEngineResult v_alloc_image_view(VkImage image, VkFormat format, VkImageViewCreateFlags flags, VkImageView *pImageView);

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
