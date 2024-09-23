#include "v_mem.h"

#include "context.h"

#include "SDL_log.h"

const Vertex vertices[6] = {
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

VEngineResult v_alloc_vertex_buffer() {
    VEngineResult buffer_result;
    VkResult result;

    buffer_result = v_alloc_buffer(
                sizeof(vertices),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &context.vk.vertexBuffer,
                &context.vk.vertexBufferMemory);

    if(buffer_result.type != VE_SUCCESS) {
        return buffer_result;
    }

    void* data;
    vkMapMemory(context.vk.device, context.vk.vertexBufferMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, &vertices, sizeof(vertices));
    vkUnmapMemory(context.vk.device, context.vk.vertexBufferMemory);

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
