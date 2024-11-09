#ifndef U_CONFIG_29
#define U_CONFIG_29

#include "u_config_def.h"
#include "context.h"

void u_config_defaults(UConfig *this);

void u_config_bound(UConfig *this);

int u_config_gather_vulkan_devices(UConfig *this, uint32_t physicalDeviceCount, VkPhysicalDevice *pPhysicalDevices);

int u_config_gather_vulkan_limits(UConfig *this, VkPhysicalDevice physicalDevice);

int u_config_load(UConfig *this, const char *const pUTF8Filepath);

int u_config_save(const UConfig *const this, const char *const pUTF8Filepath);

#endif // U_CONFIG_29
