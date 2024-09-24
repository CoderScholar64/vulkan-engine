#include "v_render.h"

#include "context.h"
#include "v_mem.h"

VEngineResult v_draw_frame(float delta) {
    const Uint64 TIME_OUT_NS = 25000000;

    VkResult result;
    Uint32 imageIndex;
    VEngineResult returnCode;
    returnCode.type  = VE_TIME_OUT;
    returnCode.point = 0;

    result = vkWaitForFences(context.vk.device, 1, &context.vk.frames[context.vk.currentFrame].inFlightFence, VK_TRUE, TIME_OUT_NS);

    if(result == VK_TIMEOUT)
        RETURN_RESULT_CODE(VE_TIME_OUT, 0) // Cancel drawing the frame then.
    else if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: in flight fence had failed with %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 0) // Program had encountered a problem!
    }

    result = vkAcquireNextImageKHR(context.vk.device, context.vk.swapChain, TIME_OUT_NS, context.vk.frames[context.vk.currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if(result == VK_TIMEOUT)
        RETURN_RESULT_CODE(VE_TIME_OUT, 1) // Cancel drawing the frame then.
    else if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        returnCode = v_recreate_swap_chain();

        // Cancel drawing the frame as well.
        if(returnCode.type == VE_SUCCESS)
            RETURN_RESULT_CODE(VE_TIME_OUT, 2)

        return returnCode;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: failed vkAcquireNextImageKHR with code %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 1) // Program had encountered a problem!
    }

    // vkResultFences returns something, but ignoring it.
    vkResetFences(context.vk.device, 1, &context.vk.frames[context.vk.currentFrame].inFlightFence);

    vkResetCommandBuffer(context.vk.frames[context.vk.currentFrame].commandBuffer, 0);

    v_update_uniform_buffer(delta, context.vk.currentFrame);

    v_record_command_buffer(context.vk.frames[context.vk.currentFrame].commandBuffer, imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &context.vk.frames[context.vk.currentFrame].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &context.vk.frames[context.vk.currentFrame].commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context.vk.frames[context.vk.currentFrame].renderFinishedSemaphore;

    result = vkQueueSubmit(context.vk.graphicsQueue, 1, &submitInfo, context.vk.frames[context.vk.currentFrame].inFlightFence);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: failed to submit to queue code %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 2) // Program had encountered a problem!
    }

    VkPresentInfoKHR presentInfo;
    memset(&presentInfo, 0, sizeof(presentInfo));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &context.vk.frames[context.vk.currentFrame].renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &context.vk.swapChain;
    presentInfo.pImageIndices  = &imageIndex;

    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(context.vk.presentationQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context.forceSwapChainRegen == 1) {
        context.forceSwapChainRegen = 0;
        returnCode = v_recreate_swap_chain();
    }
    else if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: queue present failed %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 3)
    }

    context.vk.currentFrame = (context.vk.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkResult result;

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    memset(&commandBufferBeginInfo, 0, sizeof(commandBufferBeginInfo));
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0; // OPTIONAL
    commandBufferBeginInfo.pInheritanceInfo = NULL; // OPTIONAL

    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_RECORD_COMMAND_BUFFER_FAILURE, 0)
    }

    VkRenderPassBeginInfo renderPassBeginInfo;
    memset(&renderPassBeginInfo, 0, sizeof(renderPassBeginInfo));
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = context.vk.renderPass;
    renderPassBeginInfo.framebuffer = context.vk.pSwapChainFrames[imageIndex].framebuffer;

    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = context.vk.swapExtent;

    VkClearValue clearColor;
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.0f;
    clearColor.color.float32[2] = 0.0f;
    clearColor.color.float32[3] = 1.0f;

    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.vk.graphicsPipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float)context.vk.swapExtent.width;
    viewport.height = (float)context.vk.swapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = context.vk.swapExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {context.vk.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, context.vk.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.vk.pipelineLayout, 0, 1, &context.vk.frames[context.vk.currentFrame].descriptorSet, 0, NULL);

    vkCmdDrawIndexed(commandBuffer, sizeof(builtin_indexes) / sizeof(builtin_indexes[0]), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_RECORD_COMMAND_BUFFER_FAILURE, 1)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

void v_update_uniform_buffer(float delta, uint32_t imageIndex) {
    // TODO All of this is temporary.
    static float time = 0;
    static UniformBufferObject ubo;

    Vector3 up     = {0.0f, 0.0f, 1.0f};
    Vector3 eye    = {2.0f, 2.0f, 2.0f};
    Vector3 target = {0.0f, 0.0f, 0.0f};

    time += delta;

    ubo.model = MatrixRotate(up, (90.0 * DEG2RAD) * time);

    ubo.view  = MatrixTranslate(0.0f, 0.0f, -0.5f); //MatrixLookAt(eye, target, up);

    ubo.proj  = MatrixScale(context.vk.swapExtent.height / (float) context.vk.swapExtent.width, 1.0f, 1.0f); //MatrixPerspective(45.0 * DEG2RAD, context.vk.swapExtent.width / (float) context.vk.swapExtent.height, 0.1f, 10.0f);

    memcpy(context.vk.frames[imageIndex].uniformBufferMapped, &ubo, sizeof(ubo));
}
