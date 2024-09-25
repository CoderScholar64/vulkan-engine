#include "v_mem.h"

#include "context.h"

#include "SDL_log.h"

const Vertex builtin_vertices[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
};

const Uint16 builtin_indexes[6] = {
    0, 1, 2, 2, 3, 0
};

const VkVertexInputBindingDescription vertexBindingDescription = {
//  binding,         stride,                   inputRate
          0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX
};

const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] = {
//   location, binding,                     format,                 offset
    {       0,       0,    VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex,   pos)},
    {       1,       0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
};

VEngineResult v_alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory) {
    VkResult result;

    VkBufferCreateInfo bufferCreateInfo;
    memset(&bufferCreateInfo, 0, sizeof(bufferCreateInfo));
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(context.vk.device, &bufferCreateInfo, NULL, pBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateBuffer failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 0)
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.vk.device, *pBuffer, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo;
    memset(&memoryAllocateInfo, 0, sizeof(memoryAllocateInfo));
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = v_find_memory_type_index(memRequirements.memoryTypeBits, propertyFlags);

    if(memoryAllocateInfo.memoryTypeIndex == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Memory type not found");
        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 1)
    }
    else
        memoryAllocateInfo.memoryTypeIndex--;

    result = vkAllocateMemory(context.vk.device, &memoryAllocateInfo, NULL, pBufferMemory);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory failed with result: %i", result);

        vkDestroyBuffer(context.vk.device, *pBuffer, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 2)
    }

    result = vkBindBufferMemory(context.vk.device, *pBuffer, *pBufferMemory, 0);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBindBufferMemory failed with result: %i", result);

        vkDestroyBuffer(context.vk.device, *pBuffer, NULL);
        vkFreeMemory(context.vk.device, *pBufferMemory, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 3)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_static_buffer(const void *pData, size_t sizeOfData, VkBuffer *pBuffer, VkBufferUsageFlags usageFlags, VkDeviceMemory *pBufferMemory) {
    VEngineResult bufferResult;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferResult = v_alloc_buffer(
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                &stagingBufferMemory);

    if(bufferResult.type != VE_SUCCESS) {
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, bufferResult.point)
    }

    void* pDstData;
    vkMapMemory(context.vk.device, stagingBufferMemory, 0, sizeOfData, 0, &pDstData);
    memcpy(pDstData, pData, sizeOfData);
    vkUnmapMemory(context.vk.device, stagingBufferMemory);

    bufferResult = v_alloc_buffer(
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                pBuffer,
                pBufferMemory);

    if(bufferResult.type != VE_SUCCESS) {
        vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
        vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, 4 + bufferResult.point)
    }

    bufferResult = v_copy_buffer(stagingBuffer, 0, *pBuffer, 0, sizeOfData);

    vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
    vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);

    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer failed with result: %i", bufferResult.type);
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, 8)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_builtin_vertex_buffer() {
    return v_alloc_static_buffer(&builtin_vertices, sizeof(builtin_vertices), &context.vk.vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &context.vk.vertexBufferMemory);
}

VEngineResult v_alloc_builtin_index_buffer() {
    return v_alloc_static_buffer(&builtin_indexes, sizeof(builtin_indexes), &context.vk.indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &context.vk.indexBufferMemory);
}

VEngineResult v_alloc_builtin_uniform_buffers() {
    VEngineResult bufferResult;

    for(Uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        bufferResult = v_alloc_buffer(
            sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &context.vk.frames[i].uniformBuffer,
            &context.vk.frames[i].uniformBufferMemory);

        if(bufferResult.type != VE_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer failed with result: %i", bufferResult.type);
            return bufferResult;
        }

        vkMapMemory(context.vk.device, context.vk.frames[i].uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &context.vk.frames[i].uniformBufferMapped);
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_image(Uint32 width, Uint32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory) {
    VkResult result;

    VkImageCreateInfo imageCreateInfo;
    memset(&imageCreateInfo, 0, sizeof(imageCreateInfo));
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = 0;

    result = vkCreateImage(context.vk.device, &imageCreateInfo, NULL, pImage);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateImage had failed with %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 0)
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context.vk.device, *pImage, &memRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = NULL;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = v_find_memory_type_index(memRequirements.memoryTypeBits, properties);

    if(memoryAllocateInfo.memoryTypeIndex == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Memory type not found");
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 1)
    }
    else
        memoryAllocateInfo.memoryTypeIndex--;

    result = vkAllocateMemory(context.vk.device, &memoryAllocateInfo, NULL, pImageMemory);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory had failed with %i", result);

        vkDestroyImage(context.vk.device, *pImage, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 2)
    }

    result = vkBindImageMemory(context.vk.device, *pImage, *pImageMemory, 0);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateMemory had failed with %i", result);

        vkDestroyImage(context.vk.device, *pImage, NULL);
        vkFreeMemory(context.vk.device, *pImageMemory, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_FAILURE, 3)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_copy_buffer(VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size) {
    VEngineResult bufferResult;

    VkCommandBuffer commandBuffer;

    bufferResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 0)
    }

    VkBufferCopy copyRegion;
    memset(&copyRegion, 0, sizeof(copyRegion));
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    bufferResult = v_end_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_begin_one_time_command_buffer(VkCommandBuffer *pCommandBuffer) {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    memset(&commandBufferAllocateInfo, 0, sizeof(commandBufferAllocateInfo));
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = context.vk.commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(context.vk.device, &commandBufferAllocateInfo, pCommandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateCommandBuffers failed with result: %i", result);
        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 0)
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    memset(&commandBufferBeginInfo, 0, sizeof(commandBufferBeginInfo));
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(*pCommandBuffer, &commandBufferBeginInfo);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_end_one_time_command_buffer(VkCommandBuffer *pCommandBuffer) {
    VkResult result = vkEndCommandBuffer(*pCommandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEndCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 2)
    }

    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = pCommandBuffer;

    result = vkQueueSubmit(context.vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 3)
    }

    result = vkQueueWaitIdle(context.vk.graphicsQueue);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, pCommandBuffer);

        RETURN_RESULT_CODE(VE_1_TIME_COMMAND_BUFFER_FAILURE, 4)
    }

    vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, pCommandBuffer);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VEngineResult bufferResult;

    VkImageMemoryBarrier imageMemoryBarrier;
    memset(&imageMemoryBarrier, 0, sizeof(imageMemoryBarrier));
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.srcAccessMask = 0; // TODO For some reason this is there?
    imageMemoryBarrier.dstAccessMask = 0;

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {}
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {}
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "unsupported layout transition");
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 0)
    }

    VkCommandBuffer commandBuffer;

    bufferResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 1)
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        0, 0, // Both TODO For some reason this is there?
        0,
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier
    );

    bufferResult = v_end_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 2)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_copy_buffer_to_image(VkBuffer buffer, VkImage image, Uint32 width, Uint32 height) {
    VEngineResult bufferResult;

    VkCommandBuffer commandBuffer;

    bufferResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 0)
    }

    VkBufferImageCopy bufferImageCopy;
    memset(&bufferImageCopy, 0, sizeof(bufferImageCopy));
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;

    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;

    bufferImageCopy.imageOffset.x = 0;
    bufferImageCopy.imageOffset.y = 0;
    bufferImageCopy.imageOffset.z = 0;

    bufferImageCopy.imageExtent.width  = width;
    bufferImageCopy.imageExtent.height = height;
    bufferImageCopy.imageExtent.depth  = 1;

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferImageCopy
    );

    bufferResult = v_end_one_time_command_buffer(&commandBuffer);
    if(bufferResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", bufferResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 1)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

Uint32 v_find_memory_type_index(Uint32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(context.vk.physicalDevice, &physicalDeviceMemoryProperties);

    for(Uint32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) != 0 && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i + 1;
    }

    return 0;
}
