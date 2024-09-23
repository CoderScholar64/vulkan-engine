#ifndef V_RENDER_29
#define V_RENDER_29

#include "v_init.h"

#include <vulkan/vulkan.h>

VEngineResult v_draw_frame();

VEngineResult v_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

#endif // V_RENDER_29
