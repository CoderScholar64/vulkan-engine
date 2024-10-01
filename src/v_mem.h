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
 * This function allocates a built-in uniform buffer to the context.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this buffer is successfully created. If VE_ALLOC_MEMORY_V_BUFFER_FAILURE then the buffer had failed to generate.
 */
VEngineResult v_alloc_builtin_uniform_buffers();

VEngineResult v_load_model(const char *const pUTF8Filepath);

/**
 * This function allocates an image.
 * @warning Make sure that v_init() is called first.
 * @param width The width of the new image in pixel units.
 * @param height The height of the new image in pixel units.
 * @param mipLevels The amount of mipmap levels that the mipmap in the buffer has. If the image has no mipmaps then set this to one.
 * @param numSamples The number of samples that this image supports
 * @param format The format of the new image. @warning Be sure that it is valid for the device.
 * @param tiling How would the image be laid out. @warning Be sure that it is valid for the device and format.
 * @param usage What would the image be used for in the flags. @note See VkImageUsageFlags for details
 * @param properties The desired property flags of the memory type to be allocated for.
 * @param pImage The image that would be generated. @warning This must point to a VkImage that is not initialized yet.
 * @param pImageMemory The memory where the image would reside. @warning This must point to a VkDeviceMemory that is not initialized yet.
 * @return A VEngineResult. If its type is VE_SUCCESS then srcBuffer is successfully copied to dstBuffer. If VE_ALLOC_IMAGE_FAILURE then Vulkan had found a problem
 */
VEngineResult v_alloc_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory);

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
 * @warning Be sure that pCommandBuffer points to a VkCommandBuffer that is allocated from v_begin_one_time_command_buffer().
 * @param pCommandBuffer the pointer to an ALLOCATED command buffer.
 * @return A VEngineResult. If its type is VE_SUCCESS then command buffer has been submitted and deleted. If VE_1_TIME_COMMAND_BUFFER_FAILURE then the command buffer submit/deletion process encountered a problem.
 */
VEngineResult v_end_one_time_command_buffer(VkCommandBuffer *pCommandBuffer);

/**
 * This function transfers image layouts. For example, it would convert a linearly stored image to an image optimized for random access.
 * @warning Make sure that v_init() is called first.
 * @param image the image where the layout would be converted.
 * @param format the format of the image to be converted.
 * @param oldLayout the old layout being used by the image.
 * @param newLayout The new layout that the image should be using.
 * @param mipLevels The amount of mipmap levels that the mipmap in the buffer has. If the image has no mipmaps then set this to one.
 * @return A VEngineResult. If its type is VE_SUCCESS then the image uses the newLayout. If VE_TRANSIT_IMAGE_LAYOUT_FAILURE then a problem occured. If point is zero then this function does not support the specific oldLayout and newLayout configuration.
 */
VEngineResult v_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

/**
 * This transfers buffer information to the image.
 * @warning Make sure that v_init() is called first.
 * @param buffer The buffer where the image data is stored.
 * @param image An image where the buffer would be copied to.
 * @param width The width of the image in pixel units.
 * @param height The height of the image in pixel units.
 * @param primeImageSize The size of the first image in the mip map.
 * @param mipLevels The amount of mipmap levels that the mipmap in the buffer has. If the image has no mipmaps then set this to one.
 * @return A VEngineResult. If its type is VE_SUCCESS then the image has been copied to the image. If VE_COPY_BUFFER_TO_IMAGE_FAILURE then Vulkan had found a problem.
 */
VEngineResult v_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDeviceSize primeImageSize, uint32_t mipLevels);

/**
 * This function allocates an image view.
 * @param format The format of the image parameter.
 * @param createFlags These are additional flags for the image view. Setting this value other than 0 will require extensions, so the suggested value 0. @note See VkImageViewCreateFlags for details
 * @param aspectFlags This specifies how the image in the view would be used. E.g. VK_IMAGE_ASPECT_COLOR_BIT for color or VK_IMAGE_ASPECT_DEPTH_BIT. @note See VkImageAspectFlags for details.
 * @param pImageView The image view that would be generated. @warning This must point to a VkImageView that is not initialized yet.
 * @param mipLevels The amount of mipmap levels that the mipmap in the buffer has. If the image has no mipmaps then set this to one.
 * @return A VEngineResult. If its type is VE_SUCCESS then the image view is successfully created. If VE_ALLOC_IMAGE_VIEW_FAILURE then Vulkan had found a problem
 */
VEngineResult v_alloc_image_view(VkImage image, VkFormat format, VkImageViewCreateFlags createFlags, VkImageAspectFlags aspectFlags, VkImageView *pImageView, uint32_t mipLevels);

/**
 * Find the memory buffer from the device.
 * @warning Make sure that v_init() is called first.
 * @note If you get a non-zero return you would need to subtract it by one to get the actual index.
 * @param typeFilter The memory type bits as needed from the VkMemoryRequirements::memoryTypeBits.
 * @param properties The desired property flags of the memory index.
 * @return If zero then this function failed to find the memory type with the properties and typeFilter specificed.
 */
uint32_t v_find_memory_type_index(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

VkSampleCountFlagBits v_find_closet_flag_bit(VkSampleCountFlagBits flags);

#endif // V_MEMORY_29
