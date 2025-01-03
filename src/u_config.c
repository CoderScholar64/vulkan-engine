#include "u_config.h"

#include <stdio.h>

#include "iniparser.h"

static const char UUID_FORMAT[] = "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x";

static void snprintUUID(char* pTextBuffer, size_t textBufferSize, const uint8_t uuid[VK_UUID_SIZE]);

void u_config_defaults(UConfig *this) {
    this->current.width = 1024;
    this->current.height = 764;
    this->current.sampleCount = 1;

    for(unsigned i = 0; i < VK_UUID_SIZE; i++) {
        this->current.graphicsCardPipelineCacheUUID[i] = 0;
    }
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

    if(this->current.sampleCount < this->min.sampleCount)
        this->current.sampleCount = this->min.sampleCount;
    else
    if(this->current.sampleCount > this->max.sampleCount)
        this->current.sampleCount = this->max.sampleCount;
}

int u_config_gather_vulkan_devices(UConfig *this, uint32_t physicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) {
    char textBuffer[256];
    VkPhysicalDeviceProperties physicalDeviceProperties;

    UConfigParameters *pMin = &this->min;
    UConfigParameters *pMax = &this->max;

    for(unsigned i = 0; i < VK_UUID_SIZE; i++) {
        pMin->graphicsCardPipelineCacheUUID[i] = 0x00;
        pMax->graphicsCardPipelineCacheUUID[i] = 0xff;
    }

    for(uint32_t i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(pPhysicalDevices[i], &physicalDeviceProperties);

        snprintUUID(textBuffer, sizeof(textBuffer) / sizeof(textBuffer[0]), physicalDeviceProperties.pipelineCacheUUID);

        SDL_Log( "[%i] %s %s", i, textBuffer, physicalDeviceProperties.deviceName);
    }

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

    pMin->sampleCount = VK_SAMPLE_COUNT_64_BIT;
    pMax->sampleCount = VK_SAMPLE_COUNT_1_BIT;
    for(int i = VK_SAMPLE_COUNT_1_BIT; i <= VK_SAMPLE_COUNT_64_BIT; i = i << 1) {
        if( (physicalDeviceProperties.limits.framebufferColorSampleCounts & i) != 0 &&
            (physicalDeviceProperties.limits.framebufferDepthSampleCounts & i) != 0 )
        {
            if(pMin->sampleCount > i) {
                pMin->sampleCount = i;
            }
            if(pMax->sampleCount < i) {
                pMax->sampleCount = i;
            }
        }
    }

    if(pMin->sampleCount > pMax->sampleCount) {
        // Disable anti aliasing.
        pMin->sampleCount = 1;
        pMax->sampleCount = 1;

        return 0;
    }

    return 1;
}

int u_config_load(UConfig *this, const char *const pPath) {
    FILE *pData = fopen(pPath, "r");

    if(pData == NULL) {
        return 0;
    }

    dictionary *pDictionary = iniparser_load_file(pData, pPath);

    this->current.width  = iniparser_getint(pDictionary, "window:width",   this->min.width);
    this->current.height = iniparser_getint(pDictionary, "window:height",  this->min.height);

    this->current.sampleCount = iniparser_getint(pDictionary, "window:sample_count", this->min.sampleCount);

    const char *pUUIDString = iniparser_getstring(pDictionary, "window:graphics_card_pipeline_cache_uuid", NULL);

    unsigned uuid[VK_UUID_SIZE] = {0};

    if(pUUIDString != NULL) {
        sscanf(pUUIDString, UUID_FORMAT,
            &uuid[ 0], &uuid[ 1], &uuid[ 2], &uuid[ 3],
            &uuid[ 4], &uuid[ 5],
            &uuid[ 6], &uuid[ 7],
            &uuid[ 8], &uuid[ 9],
            &uuid[10], &uuid[11], &uuid[12], &uuid[13], &uuid[14], &uuid[15]);
    }

    for(unsigned i = 0; i < VK_UUID_SIZE; i++) {
        this->current.graphicsCardPipelineCacheUUID[i] = uuid[i];
    }

    return 1;
}

int u_config_save(const UConfig *const this, const char *const pPath) {
    FILE *pData = fopen(pPath, "w");
    char textBuffer[256];

    if(pData == NULL) {
        return 0;
    }

    dictionary *pDictionary = iniparser_load_file(pData, pPath);

    if(pDictionary == NULL)
        return -1;

    iniparser_set(pDictionary, "window", NULL);

    snprintf(textBuffer, sizeof(textBuffer) / sizeof(textBuffer[0]), "%i", this->current.width);
    iniparser_set(pDictionary, "window:width", textBuffer);

    snprintf(textBuffer, sizeof(textBuffer) / sizeof(textBuffer[0]), "%i", this->current.height);
    iniparser_set(pDictionary, "window:height", textBuffer);

    snprintf(textBuffer, sizeof(textBuffer) / sizeof(textBuffer[0]), "%i", this->current.sampleCount);
    iniparser_set(pDictionary, "window:sample_count", textBuffer);

    snprintUUID(textBuffer, sizeof(textBuffer) / sizeof(textBuffer[0]), this->current.graphicsCardPipelineCacheUUID);
    iniparser_set(pDictionary, "window:graphics_card_pipeline_cache_uuid", textBuffer);

    iniparser_dump_ini(pDictionary, pData);
    fclose(pData);
    iniparser_freedict(pDictionary);
    return 1;
}

static void snprintUUID(char* pTextBuffer, size_t textBufferSize, const uint8_t uuid[VK_UUID_SIZE]) {
    snprintf(pTextBuffer, textBufferSize, UUID_FORMAT,
        uuid[ 0], uuid[ 1], uuid[ 2], uuid[ 3],
        uuid[ 4], uuid[ 5],
        uuid[ 6], uuid[ 7],
        uuid[ 8], uuid[ 9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}
