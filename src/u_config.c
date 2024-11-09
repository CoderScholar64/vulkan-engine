#include "u_config.h"

void u_config_defaults(UConfig *this) {
    this->current.width = 1024;
    this->current.height = 764;
    this->current.sample_count = 1;
    this->current.graphics_card_index = -1;
    this->current.pixel_format_index  = -1;
}

int u_config_gather_vulkan_devices(UConfig *this, const Context *const pContext) {
    //
}

int u_config_gather_vulkan_limits(UConfig *this, const Context *const pContext) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    UConfigParameters *pMin = &this->min;
    UConfigParameters *pMax = &this->max;

    vkGetPhysicalDeviceProperties(pContext->vk.physicalDevice, &physicalDeviceProperties);

    pMin->width  = 1;
    pMin->height = 1;
    pMax->width  = physicalDeviceProperties.limits.maxFramebufferWidth;
    pMax->height = physicalDeviceProperties.limits.maxFramebufferHeight;

    pMin->sample_count = VK_SAMPLE_COUNT_64_BIT;
    pMax->sample_count = VK_SAMPLE_COUNT_1_BIT;
    for(int i = VK_SAMPLE_COUNT_1_BIT; i <= VK_SAMPLE_COUNT_64_BIT; i = i << 1) {
        if( (physicalDeviceProperties.limits.framebufferColorSampleCounts & i) != 0 &&
            (physicalDeviceProperties.limits.framebufferDepthSampleCounts & i) != 0 )
        {
            if(pMin->sample_count > i) {
                pMin->sample_count = i;
            }
            if(pMax->sample_count < i) {
                pMax->sample_count = i;
            }
        }
    }

    if(pMin->sample_count > pMax->sample_count) {
        // Disable anti aliasing.
        pMin->sample_count = 1;
        pMax->sample_count = 1;

        return 0;
    }

    return 1;
}

int u_config_load(UConfig *this, const char *const pUTF8Filepath) {
    //
}

int u_config_save(const UConfig *const this, const char *const pUTF8Filepath) {
    //
}
