#include "u_config.h"

#include <stdio.h>

#include "iniparser.h"

void u_config_defaults(UConfig *this) {
    this->current.width = 1024;
    this->current.height = 764;
    this->current.sample_count = 1;
    this->current.graphics_card_index = -1;
    this->current.pixel_format_index  = -1;

    u_config_bound(this);
}

void u_config_bound(UConfig *this) {
    if(this->current.width < this->min.width)
        this->current.width = this->min.width;
    else
    if(this->current.width > this->max.width)
        this->current.width = this->max.width;

    if(this->current.height < this->min.height)
        this->current.height = this->min.height;
    else
    if(this->current.height > this->max.height)
        this->current.height = this->max.height;

    if(this->current.sample_count < this->min.sample_count)
        this->current.sample_count = this->min.sample_count;
    else
    if(this->current.sample_count > this->max.sample_count)
        this->current.sample_count = this->max.sample_count;
}

int u_config_gather_vulkan_devices(UConfig *this, uint32_t physicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) {
    UConfigParameters *pMin = &this->min;
    UConfigParameters *pMax = &this->max;

    pMin->graphics_card_index = 0;
    pMax->graphics_card_index = physicalDeviceCount;

    return 1;
}

int u_config_gather_vulkan_limits(UConfig *this, VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    UConfigParameters *pMin = &this->min;
    UConfigParameters *pMax = &this->max;

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

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

int u_config_load(UConfig *this, const char *const pPath) {
    u_config_bound(this);
}

int u_config_save(const UConfig *const this, const char *const pPath) {
    FILE *pData = fopen(pPath, "w");

    if(pData == NULL) {
        return 0;
    }

    dictionary *pDictionary = iniparser_load_file(pData, pPath);

    if(pDictionary == NULL)
        return -1;

    iniparser_set(pDictionary, "WINDOW", NULL);
    iniparser_set(pDictionary, "WINDOW:width", "this->current.width");
    iniparser_set(pDictionary, "WINDOW:height", "this->current.height");
    iniparser_set(pDictionary, "WINDOW:sample_count", "this->current.sample_count");

    iniparser_dump_ini(pDictionary, pData);
    fclose(pData);
    iniparser_freedict(pDictionary);
    return 1;
}
