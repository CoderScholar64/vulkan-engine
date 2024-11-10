#ifndef U_CONFIG_DEF_29
#define U_CONFIG_DEF_29

#include <vulkan/vulkan.h>

typedef struct UConfigParameters {
    int width;
    int height;
    int sampleCount;
    uint8_t graphicsCardPipelineCacheUUID[VK_UUID_SIZE];
} UConfigParameters;

typedef struct UConfig {
    UConfigParameters min;
    UConfigParameters current;
    UConfigParameters max;
} UConfig;

#endif // U_CONFIG_DEF_29
