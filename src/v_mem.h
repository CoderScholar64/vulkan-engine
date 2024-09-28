#ifndef V_MEMORY_29
#define V_MEMORY_29

#include "v_results.h"

#include "raymath.h"
#include "SDL_stdinc.h"
#include <vulkan/vulkan.h>

typedef struct {
    Vector3 pos;
    Vector3 color;
    Vector2 texCoord;
} Vertex;

typedef struct {
    Matrix model;
    Matrix view;
    Matrix proj;
} UniformBufferObject;


extern const Vertex builtin_vertices[8];
extern const Uint16 builtin_indexes[12];

extern const VkVertexInputBindingDescription vertexBindingDescription;
extern const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[3];

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

/**
 * This function allocates an image.
 * @warning Make sure that v_init() is called first.
 * @param width The width of the new image in pixel units.
 * @param height The height of the new image in pixel units.
 * @param format The format of the new image. @warning Be sure that it is valid for the device.
 * @param tiling How would the image be laid out. @warning Be sure that it is valid for the device and format.
 * @param usage What would the image be used for in the flags. @note See VkImageUsageFlags for details
 * @param properties The desired property flags of the memory type to be allocated for.
 * @param pImage The image that would be generated. @warning This must point to a VkImage that is not initialized yet.
 * @param pImageMemory The memory where the image would reside. @warning This must point to a VkDeviceMemory that is not initialized yet.
 * @return A VEngineResult. If its type is VE_SUCCESS then srcBuffer is successfully copied to dstBuffer. If VE_ALLOC_IMAGE_FAILURE then Vulkan had found a problem
 */
VEngineResult v_alloc_image(Uint32 width, Uint32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory);

/**
 * This function copies the srcBuffer to dstBuffer via Vulkan.
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
 * This is a convience function to make one time command buffers when needed.
 * @warning Make sure that v_init() is called first.
 * @param pCommandBuffer the pointer to an unallocated command buffer. @warning Make sure it points to an empty handle VkCommandBuffer!
 * @return A VEngineResult. If its type is VE_SUCCESS then the command buffer has been allocated. If VE_1_TIME_COMMAND_BUFFER_FAILURE then the command buffer creation process encountered a problem.
 */
VEngineResult v_begin_one_time_command_buffer(VkCommandBuffer *pCommandBuffer);

/**
 * This is a convience function to end the one time command buffer allocated from v_begin_one_time_command_buffer().
 * @warning Make sure that v_init() is called first and pCommandBuffer being the same value from v_begin_one_time_command_buffer
 * @param pCommandBuffer the pointer to an ALLOCATED command buffer.
 * @return A VEngineResult. If its type is VE_SUCCESS then command buffer has been submitted and deleted. If VE_1_TIME_COMMAND_BUFFER_FAILURE then the command buffer submit/deletion process encountered a problem.
 */
VEngineResult   v_end_one_time_command_buffer(VkCommandBuffer *pCommandBuffer);

VEngineResult v_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
VEngineResult v_copy_buffer_to_image(VkBuffer buffer, VkImage image, Uint32 width, Uint32 height);

VEngineResult v_alloc_image_view(VkImage image, VkFormat format, VkImageViewCreateFlags createFlags, VkImageAspectFlags aspectFlags, VkImageView *pImageView);

/**
 * Find the memory buffer from
 * @warning Make sure that v_init() is called first.
 * @note If you get a non-zero return you would need to subtract it by one to get the actual index.
 * @param typeFilter The memory type bits as needed from the VkMemoryRequirements::memoryTypeBits.
 * @param properties The desired property flags of the memory index.
 * @return If zero then this function failed to find the memory type with the properties and typeFilter specificed.
 */
Uint32 v_find_memory_type_index(Uint32 typeFilter, VkMemoryPropertyFlags properties);

/**
 * Find if the device supports a VkFormat with VkImageTiling and a certain VkFormatFeatureFlags.
 * @warning Make sure that v_init() is called first.
 * @param pCandidates A pointer to an array of VkFormats.
 * @param candidateAmount The amount of VkFormats that pCandiates contains.
 * @param tiling Whether the format supports the tilling mode.
 * @param features A series of FormatFeatureFlags that are enabled.
 * @return VkFormat from pCandidates if nothing is found then VK_FORMAT_UNDEFINED is returned.
 */
VkFormat v_find_supported_format(const VkFormat *const pCandidates, unsigned candidateAmount, VkImageTiling tiling, VkFormatFeatureFlags features);

/**
 * @return If the format has a stencil component return VK_TRUE or else VK_FALSE.
 */
VkBool32 v_has_stencil_component(VkFormat format);

#endif // V_MEMORY_29
