#include "v_mem.h"

#include "context.h"

#include "SDL_log.h"

const Vertex builtin_vertices[6] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}
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

VEngineResult v_alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory) {
    VkResult result;

    VkBufferCreateInfo bufferCreateInfo;
    memset(&bufferCreateInfo, 0, sizeof(bufferCreateInfo));
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
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
    memoryAllocateInfo.memoryTypeIndex = v_find_memory_type_index(memRequirements.memoryTypeBits, properties);

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

VEngineResult v_alloc_builtin_vertex_buffer() {
    VEngineResult buffer_result;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    buffer_result = v_alloc_buffer(
                sizeof(builtin_vertices),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                &stagingBufferMemory);

    if(buffer_result.type != VE_SUCCESS) {
        return buffer_result;
    }

    void* data;
    vkMapMemory(context.vk.device, stagingBufferMemory, 0, sizeof(builtin_vertices), 0, &data);
    memcpy(data, &builtin_vertices, sizeof(builtin_vertices));
    vkUnmapMemory(context.vk.device, stagingBufferMemory);

    buffer_result = v_alloc_buffer(
                sizeof(builtin_vertices),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &context.vk.vertexBuffer,
                &context.vk.vertexBufferMemory);

    if(buffer_result.type != VE_SUCCESS) {
        vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
        vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);
        return buffer_result;
    }

    buffer_result = v_copy_buffer(stagingBuffer, context.vk.vertexBuffer, sizeof(builtin_vertices));

    vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
    vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);

    if(buffer_result.type == VE_SUCCESS) {
        RETURN_RESULT_CODE(VE_SUCCESS, 0)
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer failed with result: %i", buffer_result.type);
        RETURN_RESULT_CODE(VE_ALLOC_MEMORY_V_BUFFER_FAILURE, 4)
    }
}

VEngineResult v_copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    memset(&commandBufferAllocateInfo, 0, sizeof(commandBufferAllocateInfo));
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = context.vk.commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    result = vkAllocateCommandBuffers(context.vk.device, &commandBufferAllocateInfo, &commandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateCommandBuffers failed with result: %i", result);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 0)
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    memset(&commandBufferBeginInfo, 0, sizeof(commandBufferBeginInfo));
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, &commandBuffer);

        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 1)
    }

    VkBufferCopy copyRegion;
    memset(&commandBufferAllocateInfo, 0, sizeof(commandBufferAllocateInfo));
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    result = vkEndCommandBuffer(commandBuffer);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEndCommandBuffer failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, &commandBuffer);

        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 2)
    }

    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    result = vkQueueSubmit(context.vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, &commandBuffer);

        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 3)
    }

    result = vkQueueWaitIdle(context.vk.graphicsQueue);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkQueueSubmit failed with result: %i", result);

        vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, &commandBuffer);

        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 3)
    }

    vkFreeCommandBuffers(context.vk.device, context.vk.commandPool, 1, &commandBuffer);
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
