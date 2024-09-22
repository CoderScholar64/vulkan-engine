#ifndef V_RENDER_29
#define V_RENDER_29

#include "v_init.h"

#include <vulkan/vulkan.h>

int v_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

int v_draw_frame();

#endif // V_RENDER_29
