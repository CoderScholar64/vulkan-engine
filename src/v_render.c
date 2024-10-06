#include "v_render.h"

#include "context.h"
#include "v_init.h"
#include "v_model.h"

VEngineResult v_draw_frame(Context *this, float delta) {
    const uint64_t TIME_OUT_NS = 25000000;

    VkResult result;
    uint32_t imageIndex;
    VEngineResult returnCode;
    returnCode.type  = VE_TIME_OUT;
    returnCode.point = 0;

    result = vkWaitForFences(this->vk.device, 1, &this->vk.frames[this->vk.currentFrame].inFlightFence, VK_TRUE, TIME_OUT_NS);

    if(result == VK_TIMEOUT)
        RETURN_RESULT_CODE(VE_TIME_OUT, 0) // Cancel drawing the frame then.
    else if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: in flight fence had failed with %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 0) // Program had encountered a problem!
    }

    result = vkAcquireNextImageKHR(this->vk.device, this->vk.swapChain, TIME_OUT_NS, this->vk.frames[this->vk.currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if(result == VK_TIMEOUT)
        RETURN_RESULT_CODE(VE_TIME_OUT, 1) // Cancel drawing the frame then.
    else if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        returnCode = v_recreate_swap_chain(this);

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
    vkResetFences(this->vk.device, 1, &this->vk.frames[this->vk.currentFrame].inFlightFence);

    vkResetCommandBuffer(this->vk.frames[this->vk.currentFrame].commandBuffer, 0);

    v_record_command_buffer(this, this->vk.frames[this->vk.currentFrame].commandBuffer, imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &this->vk.frames[this->vk.currentFrame].imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->vk.frames[this->vk.currentFrame].commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &this->vk.frames[this->vk.currentFrame].renderFinishedSemaphore;

    result = vkQueueSubmit(this->vk.graphicsQueue, 1, &submitInfo, this->vk.frames[this->vk.currentFrame].inFlightFence);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: failed to submit to queue code %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 2) // Program had encountered a problem!
    }

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &this->vk.frames[this->vk.currentFrame].renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &this->vk.swapChain;
    presentInfo.pImageIndices  = &imageIndex;

    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(this->vk.presentationQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->forceSwapChainRegen == 1) {
        this->forceSwapChainRegen = 0;
        returnCode = v_recreate_swap_chain(this);
    }
    else if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_draw_frame: queue present failed %i aborting!", result);
        RETURN_RESULT_CODE(VE_DRAW_FRAME_FAILURE, 3)
    }

    this->vk.currentFrame = (this->vk.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

VEngineResult v_record_command_buffer(Context *this, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkResult result;

    VkCommandBufferBeginInfo commandBufferBeginInfo = {0};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0; // OPTIONAL
    commandBufferBeginInfo.pInheritanceInfo = NULL; // OPTIONAL

    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_RECORD_COMMAND_BUFFER_FAILURE, 0)
    }

    VkRenderPassBeginInfo renderPassBeginInfo = {0};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = this->vk.renderPass;
    renderPassBeginInfo.framebuffer = this->vk.pSwapChainFrames[imageIndex].framebuffer;

    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = this->vk.swapExtent;

    VkClearValue clearValues[2];
    clearValues[0].color.float32[0] = 0.0f;
    clearValues[0].color.float32[1] = 0.0f;
    clearValues[0].color.float32[2] = 0.0f;
    clearValues[0].color.float32[3] = 1.0f;
    clearValues[1].depthStencil.depth   = 0.0f; // Far plane is 0, near plane is 1.
    clearValues[1].depthStencil.stencil = 0;

    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->vk.graphicsPipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float)this->vk.swapExtent.width;
    viewport.height = (float)this->vk.swapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = this->vk.swapExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->vk.pipelineLayout, 0, 1, &this->vk.frames[this->vk.currentFrame].descriptorSet, 0, NULL);

    PushConstantObject pushConstantObjects[2];
    {
        Vector3 position = {0, 0, 0};
        PushConstantObject pushConstantObject = v_setup_pco(this, position, this->time);
        pushConstantObjects[0] = pushConstantObject;
    }
    {
        Vector3 position = {0, 2, 0};
        PushConstantObject pushConstantObject = v_setup_pco(this, position, this->time);
        pushConstantObjects[1] = pushConstantObject;
    }

    v_record_model_draws(this, commandBuffer, &this->vk.pModels[0], sizeof(pushConstantObjects) / sizeof(pushConstantObjects[0]), pushConstantObjects);

    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkBeginCommandBuffer creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_RECORD_COMMAND_BUFFER_FAILURE, 1)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

PushConstantObject v_setup_pco(Context *this, Vector3 position, float unit90Degrees) {
    PushConstantObject pushConstantObject;
    Vector3 axis   = {0.0f, 0.0f, 1.0f};

    pushConstantObject.matrix = MatrixTranspose(MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotate(axis, (90.0 * DEG2RAD) * unit90Degrees), MatrixTranslate(position.x, position.y, position.z)), this->modelView), MatrixVulkanPerspective(45.0 * DEG2RAD, this->vk.swapExtent.width / (float) this->vk.swapExtent.height, 0.125f, 100.0f)));

    return pushConstantObject;
}
