#ifndef V_RENDER_29
#define V_RENDER_29

#include "v_mem.h"

#include <vulkan/vulkan.h>

/**
 * Draw the current frame.
 * @warning Make sure that v_init() is called first.
 * @return A VEngineResult. If its type is VE_SUCCESS then this function had drawn something on the screen. If VE_TIME_OUT then nothing is drawn, if there is any error then the vulkan instance is crashed.
 */
VEngineResult v_draw_frame(float delta);

/**
 * Record a command buffer.
 * @note This is very likely to be replace with something less built in.
 * @warning Make sure that v_init() is called first.
 * @param commandBuffer The command buffer to record to.
 * @param imageIndex The index in the swap chain to overwrite.
 * @return A VEngineResult. If its type is VE_SUCCESS then this function had drawn something on the screen otherwise VE_RECORD_COMMAND_BUFFER_FAILURE and whatever point to where it failed.
 */
VEngineResult v_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

PushConstantObject v_get_test_pco(float unit90Degrees);

#endif // V_RENDER_29
