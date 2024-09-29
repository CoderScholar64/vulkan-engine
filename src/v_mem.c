#include "v_mem.h"

#include "context.h"
#include "u_read.h"

#include "SDL_log.h"

const Vertex builtin_vertices[8] = {
    {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const uint16_t builtin_indexes[12] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

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

VEngineResult v_alloc_buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory) {
    VkResult result;

    VkBufferCreateInfo bufferCreateInfo = {0};
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

    VkMemoryAllocateInfo memoryAllocateInfo = {0};
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
    VEngineResult engineResult;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    engineResult = v_alloc_buffer(
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                &stagingBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, engineResult.point)
    }

    void* pDstData;
    vkMapMemory(context.vk.device, stagingBufferMemory, 0, sizeOfData, 0, &pDstData);
    memcpy(pDstData, pData, sizeOfData);
    vkUnmapMemory(context.vk.device, stagingBufferMemory);

    engineResult = v_alloc_buffer(
                sizeOfData,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                pBuffer,
                pBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
        vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);
        RETURN_RESULT_CODE(VE_ALLOC_STATIC_BUFFER, 4 + engineResult.point)
    }

    engineResult = v_copy_buffer(stagingBuffer, 0, *pBuffer, 0, sizeOfData);

    vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
    vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer failed with result: %i", engineResult.type);
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
    VEngineResult engineResult;

    for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        engineResult = v_alloc_buffer(
            sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &context.vk.frames[i].uniformBuffer,
            &context.vk.frames[i].uniformBufferMemory);

        if(engineResult.type != VE_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer failed with result: %i", engineResult.type);
            return engineResult;
        }

        vkMapMemory(context.vk.device, context.vk.frames[i].uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &context.vk.frames[i].uniformBufferMapped);
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_load_model(const char *const pUTF8Filepath) {
    cgltf_result result;
    cgltf_data *pModel = u_gltf_read(pUTF8Filepath, &result);

    if(result != cgltf_result_success) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find model!");
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 0)
    }

    if(pModel->meshes_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no meshes contained in it to read!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 1)
    }

    if(pModel->meshes[0].primitives_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no primitives stored in it!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 2)
    }

    if(pModel->buffers_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no buffers!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 3)
    }

    if(pModel->meshes[0].primitives[0].attributes_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This glTF file has no attributes!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 4)
    }

    cgltf_size loadBufferSize = 0;

    cgltf_attribute *pPositionAttribute = NULL;
    cgltf_attribute *pColorAttribute    = NULL;
    cgltf_attribute *pTexCoordAttribute = NULL;

    for(size_t i = 0; i < pModel->meshes[0].primitives[0].attributes_count; i++) {
        switch(pModel->meshes[0].primitives[0].attributes[i].type) {
            case cgltf_attribute_type_position:
                pPositionAttribute = &pModel->meshes[0].primitives[0].attributes[i];

                SDL_Log("Position Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pPositionAttribute->data, NULL, 0));

                loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pPositionAttribute->data, NULL, 0));
                break;
            case cgltf_attribute_type_normal:
                pColorAttribute = &pModel->meshes[0].primitives[0].attributes[i];

                SDL_Log("Color Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pColorAttribute->data, NULL, 0));

                loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pColorAttribute->data, NULL, 0));
                break;
            case cgltf_attribute_type_texcoord:
                pTexCoordAttribute = &pModel->meshes[0].primitives[0].attributes[i];

                SDL_Log("Texture Buffer size = %li", sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pTexCoordAttribute->data, NULL, 0));

                loadBufferSize = fmax(loadBufferSize, sizeof(cgltf_float) * cgltf_accessor_unpack_floats(pTexCoordAttribute->data, NULL, 0));
                break;
            default:
                // Just do nothing for unrecognized attributes.
        }
        SDL_Log("Attribute %li\n  name = %s\n  type = %i\n  index = %i\n", i, pModel->meshes[0].primitives[0].attributes[i].name, pModel->meshes[0].primitives[0].attributes[i].type, pModel->meshes[0].primitives[0].attributes[i].index);
    }
    SDL_Log("Buffer size = %li", loadBufferSize);

    if(pPositionAttribute == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No position attribute found!");
        cgltf_free(pModel);
        RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 5)
    }

    cgltf_accessor* pIndices = NULL;;

    if(pModel->meshes[0].primitives[0].indices != NULL) {
        pIndices = pModel->meshes[0].primitives[0].indices;

        if(pIndices->type != cgltf_type_scalar) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This model has component_type = %i", pIndices->type);
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 6)
        }

        switch(pIndices->component_type) {
            case cgltf_component_type_r_16u:
                SDL_Log("This model has component_type = cgltf_component_type_r_16u");
                break;
            case cgltf_component_type_r_32u:
                SDL_Log("This model has component_type = cgltf_component_type_r_32u");
                break;
            default:
                SDL_Log("This model has invalid component_type = %i", pIndices->component_type);
                cgltf_free(pModel);
                RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 7)
        }

        SDL_Log("This model has indices = %li.\n", pIndices->count);

        cgltf_buffer_view *pBufferView = pIndices->buffer_view;

        if(pBufferView == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This model's indice buffer view does not exist!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 8)
        }

        cgltf_buffer *pBuffer = pBufferView->buffer;

        if(pBuffer == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This model's indice buffer view does not exist!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 9)
        }

        if(pBuffer->data == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "This model's indice buffer data does not exist!");
            cgltf_free(pModel);
            RETURN_RESULT_CODE(VE_LOAD_MODEL_FAILURE, 10)
        }

        if(pBuffer->name != NULL)
            SDL_Log("Buffer name = %s", pBuffer->name);

        if(pBuffer->uri != NULL)
            SDL_Log("Buffer uri = %s", pBuffer->uri);

        SDL_Log("BufferView offset = 0x%lx", pBufferView->offset);
        SDL_Log("BufferView size = 0x%lx", pBufferView->size);
        SDL_Log("Buffer size = 0x%lx", pBuffer->size);
    }
    //cgltf_accessor_unpack_indices(pIndices, NULL, cgltf_component_size(pIndices->component_type), pIndices->count);

    cgltf_free(pModel);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *pImage, VkDeviceMemory *pImageMemory) {
    VkResult result;

    VkImageCreateInfo imageCreateInfo = {0};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
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
    VEngineResult engineResult;

    VkCommandBuffer commandBuffer;

    engineResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 0)
    }

    VkBufferCopy copyRegion = {0};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    engineResult = v_end_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_begin_one_time_command_buffer(VkCommandBuffer *pCommandBuffer) {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = context.vk.commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(context.vk.device, &commandBufferAllocateInfo, pCommandBuffer);

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

    VkSubmitInfo submitInfo = {0};
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

        if(v_has_stencil_component(format) == VK_TRUE)
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
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

    engineResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", engineResult.point);
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

    engineResult = v_end_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_TRANSIT_IMAGE_LAYOUT_FAILURE, 2)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VEngineResult engineResult;

    VkCommandBuffer commandBuffer;

    engineResult = v_begin_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_begin_one_time_command_buffer had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 0)
    }

    VkBufferImageCopy bufferImageCopy = {0};
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

    engineResult = v_end_one_time_command_buffer(&commandBuffer);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_end_one_time_command_buffer had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_COPY_BUFFER_TO_IMAGE_FAILURE, 1)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_alloc_image_view(VkImage image, VkFormat format, VkImageViewCreateFlags createFlags, VkImageAspectFlags aspectFlags, VkImageView *pImageView) {
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
    imageViewCreateInfo.subresourceRange.levelCount     = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;

    VkResult result = vkCreateImageView(context.vk.device, &imageViewCreateInfo, NULL, pImageView);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateImageView failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_IMAGE_VIEW_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

uint32_t v_find_memory_type_index(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(context.vk.physicalDevice, &physicalDeviceMemoryProperties);

    for(uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) != 0 && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i + 1;
    }

    return 0;
}

VkFormat v_find_supported_format(const VkFormat *const pCandidates, unsigned candidateAmount, VkImageTiling tiling, VkFormatFeatureFlags features) {
    VkFormatProperties props;

    for(unsigned i = 0; i < candidateAmount; i++) {
        VkFormat format = pCandidates[ i ];

        vkGetPhysicalDeviceFormatProperties(context.vk.physicalDevice, format, &props);

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

VkBool32 v_has_stencil_component(VkFormat format) {
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
