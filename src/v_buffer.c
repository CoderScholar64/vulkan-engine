#include "v_buffer.h"

#include "context.h"
#include "u_read.h"

#include "SDL_log.h"

const VkVertexInputBindingDescription vertexBindingDescription = {
//  binding,         stride,                   inputRate
          0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX
};

const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[3] = {
//   location, binding,                     format,                    offset
    {       0,       0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,      pos)},
    {       1,       0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex,    color)},
    {       2,       0,    VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)}
};

VEngineResult v_alloc_buffer(Context *this, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory) {
    VkResult result;

    VkBufferCreateInfo bufferCreateInfo = {0};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(this->vk.device, &bufferCreateInfo, NULL, pBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateBuffer failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 0)
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(this->vk.device, *pBuffer, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {0};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = v_buffer_find_memory_type_index(this, memRequirements.memoryTypeBits, propertyFlags);

    if(memoryAllocateInfo.memoryTypeIndex == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Memory type not found");
        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 1)
    }
    else
        memoryAllocateInfo.memoryTypeIndex--;

    result = vkAllocateMemory(this->vk.device, &memoryAllocateInfo, NULL, pBufferMemory);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory failed with result: %i", result);

        vkDestroyBuffer(this->vk.device, *pBuffer, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 2)
    }

    result = vkBindBufferMemory(this->vk.device, *pBuffer, *pBufferMemory, 0);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBindBufferMemory failed with result: %i", result);

        vkDestroyBuffer(this->vk.device, *pBuffer, NULL);
        vkFreeMemory(this->vk.device, *pBufferMemory, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 3)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_static_buffer(Context *this, const void *pData, size_t sizeOfData, VkBuffer *pBuffer, VkBufferUsageFlags usageFlags, VkDeviceMemory *pBufferMemory) {
    VEngineResult engineResult;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    engineResult = v_alloc_buffer(
                this,
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                &stagingBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, engineResult.point)
    }

    void* pDstData;
    vkMapMemory(this->vk.device, stagingBufferMemory, 0, sizeOfData, 0, &pDstData);
    memcpy(pDstData, pData, sizeOfData);
    vkUnmapMemory(this->vk.device, stagingBufferMemory);

    engineResult = v_alloc_buffer(
                this,
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                pBuffer,
                pBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        vkDestroyBuffer(this->vk.device, stagingBuffer, NULL);
        vkFreeMemory(this->vk.device, stagingBufferMemory, NULL);
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, 4 + engineResult.point)
    }

    engineResult = v_buffer_copy(this, stagingBuffer, 0, *pBuffer, 0, sizeOfData);

    vkDestroyBuffer(this->vk.device, stagingBuffer, NULL);
    vkFreeMemory(this->vk.device, stagingBufferMemory, NULL);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_copy failed with result: %i", engineResult.type);
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, 8)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_builtin_uniform_buffers(Context *this) {
    VEngineResult engineResult;

    for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        engineResult = v_alloc_buffer(
            this,
            sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &this->vk.frames[i].uniformBuffer,
            &this->vk.frames[i].uniformBufferMemory);

        if(engineResult.type != VE_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_copy failed with result: %i", engineResult.type);
            return engineResult;
        }

        vkMapMemory(this->vk.device, this->vk.frames[i].uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &this->vk.frames[i].uniformBufferMapped);

        UniformBufferObject ubo = { {1, 1, 1, 1} };

        memcpy(this->vk.frames[i].uniformBufferMapped, &ubo, sizeof(ubo));
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_alloc_image(Context *this, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory) {
    VkResult result;

    VkImageCreateInfo imageCreateInfo = {0};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = numSamples;
    imageCreateInfo.flags = 0;

    result = vkCreateImage(this->vk.device, &imageCreateInfo, NULL, pImage);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateImage had failed with %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 0)
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(this->vk.device, *pImage, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = NULL;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = v_buffer_find_memory_type_index(this, memRequirements.memoryTypeBits, properties);

    if(memoryAllocateInfo.memoryTypeIndex == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Memory type not found");
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 1)
    }
    else
        memoryAllocateInfo.memoryTypeIndex--;

    result = vkAllocateMemory(this->vk.device, &memoryAllocateInfo, NULL, pImageMemory);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory had failed with %i", result);

        vkDestroyImage(this->vk.device, *pImage, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 2)
    }

    result = vkBindImageMemory(this->vk.device, *pImage, *pImageMemory, 0);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory had failed with %i", result);

        vkDestroyImage(this->vk.device, *pImage, NULL);
        vkFreeMemory(this->vk.device, *pImageMemory, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 3)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_copy(Context *this, VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size) {
    VEngineResult engineResult;

    VkCommandBuffer commandBuffer;

    engineResult = v_buffer_begin_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_begin_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 0)
    }

    VkBufferCopy copyRegion = {0};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    engineResult = v_buffer_end_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_end_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_begin_1_time_cb(Context *this, VkCommandBuffer *pCommandBuffer) {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = this->vk.commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(this->vk.device, &commandBufferAllocateInfo, pCommandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateCommandBuffers failed with result: %i", result);
        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 0)
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {0};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(*pCommandBuffer, &commandBufferBeginInfo);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(this->vk.device, this->vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_end_1_time_cb(Context *this, VkCommandBuffer *pCommandBuffer) {
    VkResult result = vkEndCommandBuffer(*pCommandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEndCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(this->vk.device, this->vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 2)
    }

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = pCommandBuffer;

    result = vkQueueSubmit(this->vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(this->vk.device, this->vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 3)
    }

    result = vkQueueWaitIdle(this->vk.graphicsQueue);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(this->vk.device, this->vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 4)
    }

    vkFreeCommandBuffers(this->vk.device, this->vk.commandPool, 1, pCommandBuffer);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_transition_image_layout(Context *this, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    VEngineResult engineResult;
    VkImageMemoryBarrier imageMemoryBarrier = {0};
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if(v_buffer_has_stencil_component(format) == VK_TRUE)
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported layout transition!");
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 0)
    }

    VkCommandBuffer commandBuffer;

    engineResult = v_buffer_begin_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_begin_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 1)
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier
    );

    engineResult = v_buffer_end_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_end_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 2)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_copy_to_image(Context *this, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDeviceSize primeImageSize, uint32_t mipLevels) {
    VEngineResult engineResult;

    VkCommandBuffer commandBuffer;

    engineResult = v_buffer_begin_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_begin_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 0)
    }

    VkDeviceSize currentImageSize = primeImageSize;

    VkBufferImageCopy bufferImageCopy = {0};
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.bufferOffset = 0;

    bufferImageCopy.imageExtent.width  = width;
    bufferImageCopy.imageExtent.height = height;
    bufferImageCopy.imageExtent.depth  = 1;

    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;

    bufferImageCopy.imageOffset.x = 0;
    bufferImageCopy.imageOffset.y = 0;
    bufferImageCopy.imageOffset.z = 0;

    for(uint32_t m = 0; m < mipLevels; m++) {
        bufferImageCopy.imageSubresource.mipLevel = m;

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bufferImageCopy
        );

        bufferImageCopy.bufferOffset += currentImageSize;
        bufferImageCopy.imageExtent.width /= 2;
        bufferImageCopy.imageExtent.height /= 2;

        currentImageSize /= 4;
    }

    engineResult = v_buffer_end_1_time_cb(this, &commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_end_1_time_cb had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 1)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_buffer_alloc_image_view(Context *this, VkImage image, VkFormat format, VkImageViewCreateFlags createFlags, VkImageAspectFlags aspectFlags, VkImageView *pImageView, uint32_t mipLevels) {
    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = NULL;
    imageViewCreateInfo.flags = createFlags;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;

    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    imageViewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;

    VkResult result = vkCreateImageView(this->vk.device, &imageViewCreateInfo, NULL, pImageView);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateImageView failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_VIEW_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

uint32_t v_buffer_find_memory_type_index(Context *this, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(this->vk.physicalDevice, &physicalDeviceMemoryProperties);

    for(uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) != 0 && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i + 1;
    }

    return 0;
}

VkFormat v_buffer_find_supported_format(Context *this, const VkFormat *const pCandidates, unsigned candidateAmount, VkImageTiling tiling, VkFormatFeatureFlags features) {
    VkFormatProperties props;

    for(unsigned i = 0; i < candidateAmount; i++) {
        VkFormat format = pCandidates[ i ];

        vkGetPhysicalDeviceFormatProperties(this->vk.physicalDevice, format, &props);

        switch(tiling) {
            case VK_IMAGE_TILING_LINEAR:
                if((props.linearTilingFeatures & features) == features)
                    return format;
                break;

            case VK_IMAGE_TILING_OPTIMAL:
                if((props.optimalTilingFeatures & features) == features)
                    return format;
                break;

            default:
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_find_supported_format: unsupported tiling value: %i", tiling);
                return VK_FORMAT_UNDEFINED;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

VkBool32 v_buffer_has_stencil_component(VkFormat format) {
    switch(format) {
        case  VK_FORMAT_S8_UINT:
        case  VK_FORMAT_D24_UNORM_S8_UINT:
        case  VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_TRUE;
        default:
            return VK_FALSE;
    }
}

VkSampleCountFlagBits v_buffer_sample_flag_bit(Context *this, VkSampleCountFlagBits flags) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(this->vk.physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    for(unsigned i = 0; (flags >> i) != 0; i++) {
        VkSampleCountFlags flag = ((flags >> i) & counts);

        if( flag != 0 )
            return flag;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}
