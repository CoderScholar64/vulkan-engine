#include "SDL.h"
#include <vulkan/vulkan.h>
#include "SDL_vulkan.h"
#include <string.h>
#include "math_utility.h"

struct Context {
    char title[64];
    int x, y;
    int w, h;
    Uint32 flags;
    SDL_Window *pWindow;

    struct {
        VkDevice device;
        VkInstance instance;
        VkQueue graphicsQueue;
        VkQueue presentationQueue;
        VkPhysicalDevice physicalDevice;
        VkQueueFamilyProperties *pQueueFamilyProperties;
        VkSurfaceKHR surface;
        Uint32 queueFamilyPropertyCount;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D swapExtent;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkSurfaceFormatKHR *pSurfaceFormat;
        Uint32 surfaceFormatCount;
        VkPresentModeKHR *pPresentMode;
        Uint32 presentModeCount;
    } vk;

} context = {"Hello World", 0, 0, 1920, 1080, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, Uint32 *pQueueFamilyPropertyCount) {
    *pQueueFamilyPropertyCount = 0;
    VkQueueFamilyProperties *pQueueFamilyProperties = NULL;

    vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, NULL);

    if(*pQueueFamilyPropertyCount != 0)
        pQueueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * (*pQueueFamilyPropertyCount));

    if(pQueueFamilyProperties != NULL)
        vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, pQueueFamilyProperties);

    return pQueueFamilyProperties;
}

VkLayerProperties* allocateLayerPropertiesArray(Uint32 *pPropertyCount) {
    *pPropertyCount = 0;
    VkLayerProperties *pLayerProperties = NULL;

    vkEnumerateInstanceLayerProperties(pPropertyCount, NULL);

    if(*pPropertyCount != 0)
        pLayerProperties = malloc(sizeof(VkLayerProperties) * (*pPropertyCount));

    if(pLayerProperties != NULL)
        vkEnumerateInstanceLayerProperties(pPropertyCount, pLayerProperties);

    return pLayerProperties;
}

int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, Uint32 requiredExtensionCount) {
    Uint32 extensionCount = 0;
    VkExtensionProperties *pExtensionProperties = NULL;
    int found;
    int everythingFound = 1;

    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);

    if(extensionCount != 0)
        pExtensionProperties = malloc(sizeof(VkExtensionProperties) * extensionCount);

    if(pExtensionProperties != NULL) {
        vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, pExtensionProperties);
    }
    else {
        return -1;
    }

    for(Uint32 r = 0; r < requiredExtensionCount; r++) {
        found = 0;

        for(Uint32 e = 0; e < extensionCount && found == 0; e++) {
            if(strcmp(ppRequiredExtension[r], pExtensionProperties[e].extensionName) == 0) {
                found = 1;
            }
        }

        if(found == 0) {
            everythingFound = 0;
            break;
        }
    }

    free(pExtensionProperties);

    return everythingFound;
}

int updateSwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    VkResult result;

    if(context.vk.surfaceFormatCount > 0)
        free(context.vk.pSurfaceFormat);

    if(context.vk.presentModeCount > 0)
        free(context.vk.pPresentMode);

    context.vk.pSurfaceFormat = NULL;
    context.vk.surfaceFormatCount = 0;
    context.vk.pPresentMode = NULL;
    context.vk.presentModeCount = 0;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &context.vk.surfaceCapabilities);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR had failed with %i", result);
        return -1;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &context.vk.surfaceFormatCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for count had failed with %i", result);
        return -2;
    }

    context.vk.pSurfaceFormat = malloc(context.vk.surfaceFormatCount * sizeof(VkSurfaceFormatKHR));

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &context.vk.surfaceFormatCount, context.vk.pSurfaceFormat);

    if(result != VK_SUCCESS || context.vk.pSurfaceFormat == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for allocation had failed with %i", result);
        return -3;
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &context.vk.presentModeCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for count had failed with %i", result);
        return -4;
    }

    context.vk.pPresentMode = malloc(context.vk.presentModeCount * sizeof(VkPresentModeKHR));

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &context.vk.presentModeCount, context.vk.pPresentMode);

    if(result != VK_SUCCESS || context.vk.pPresentMode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for allocation had failed with %i", result);
        return -5;
    }

    if(context.vk.surfaceFormatCount != 0 && context.vk.presentModeCount != 0)
        return 1;
    else
        return 0;
}

int initInstance() {
    VkApplicationInfo applicationInfo;
    memset(&applicationInfo, 0, sizeof(applicationInfo));
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = context.title;
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = NULL;
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    unsigned int extensionCount = 0;
    const char **ppExtensionNames = NULL;
    if(!SDL_Vulkan_GetInstanceExtensions(context.pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting number of extensions had failed with %s", SDL_GetError());
        return -3;
    }
    if(extensionCount != 0)
        ppExtensionNames = malloc(sizeof(char*) * extensionCount);
    if(!SDL_Vulkan_GetInstanceExtensions(context.pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting names of extensions had failed with %s", SDL_GetError());
        free(ppExtensionNames);
        return -4;
    }

    for(unsigned int i = 0; i < extensionCount; i++) {
        SDL_Log( "[%i] %s", i, ppExtensionNames[i]);
    }

    Uint32 everyLayerAmount = 0;
    VkLayerProperties* pEveryLayerPropertiesArray = allocateLayerPropertiesArray(&everyLayerAmount);
    const char * name = NULL;
    for(unsigned int i = 0; i < everyLayerAmount; i++) {
        SDL_Log( "[%i] %s", i, pEveryLayerPropertiesArray[i].layerName);

        if(strcmp(pEveryLayerPropertiesArray[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
            name = pEveryLayerPropertiesArray[i].layerName;
    }
    const char ** names = &name;

    VkInstanceCreateInfo instanceCreateInfo;
    memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    if(name == NULL)
        instanceCreateInfo.enabledLayerCount = 0;
    else
        instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = names;

    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = ppExtensionNames;

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &context.vk.instance);

    free(pEveryLayerPropertiesArray);

    if(ppExtensionNames != NULL)
        free(ppExtensionNames);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateInstance returned %i", result);
        return -4;
    }

    return 1;
}

int findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
    Uint32 physicalDevicesCount = 0;
    VkPhysicalDevice *pPhysicalDevices = NULL;

    VkResult result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, NULL);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for amount returned %i", result);
        return -5;
    }

    if(physicalDevicesCount != 0)
        pPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDevicesCount);
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "There are not any rendering device available!");
        return -6;
    }

    if(pPhysicalDevices == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "pPhysicalDevices failed to allocate %i names!", physicalDevicesCount);
        return -7;
    }

    result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, pPhysicalDevices);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for devices returned %i", result);
        free(pPhysicalDevices);
        return -8;
    }
    VkPhysicalDeviceProperties physicalDeviceProperties;

    unsigned int deviceIndex = physicalDevicesCount;
    Uint32 queueFamilyPropertyCount;
    VkQueueFamilyProperties *pQueueFamilyProperties;
    VkBool32 surfaceSupported;
    int requiredParameters;

    for(unsigned int i = physicalDevicesCount; i != 0; i--) {
        memset(&physicalDeviceProperties, 0, sizeof(physicalDeviceProperties));

        vkGetPhysicalDeviceProperties(pPhysicalDevices[i - 1], &physicalDeviceProperties);

        pQueueFamilyProperties = allocateQueueFamilyArray(pPhysicalDevices[i - 1], &queueFamilyPropertyCount);

        requiredParameters = 0;

        for(Uint32 p = 0; p < queueFamilyPropertyCount; p++) {
            if( (pQueueFamilyProperties[p].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
                requiredParameters |= 1;
            }

            result = vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevices[i - 1], p, context.vk.surface, &surfaceSupported);

            if(result != VK_SUCCESS) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "At index %i for vkGetPhysicalDeviceSurfaceSupportKHR returned %i", p - 1, result);
            }
            else if(surfaceSupported)
                requiredParameters |= 2;

            // Check for required extensions
            if(hasRequiredExtensions(pPhysicalDevices[i - 1], ppRequiredExtensions, requiredExtensionsAmount)) {
                if(updateSwapChainCapabilities(pPhysicalDevices[i - 1], context.vk.surface))
                    requiredParameters |= 4;
            }


        }

        free(pQueueFamilyProperties);

        if(requiredParameters == 7) {
            if(deviceIndex == physicalDevicesCount) {
                deviceIndex = i - 1;
            }
            else if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                deviceIndex = i - 1;
            }
        }

        SDL_Log( "[%i] %s", i - 1, physicalDeviceProperties.deviceName);
    }
    SDL_Log( "Index %i device selected", deviceIndex);

    if(deviceIndex != physicalDevicesCount) {
        context.vk.physicalDevice = pPhysicalDevices[deviceIndex];

        context.vk.pQueueFamilyProperties = allocateQueueFamilyArray(context.vk.physicalDevice, &context.vk.queueFamilyPropertyCount);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find suitable device!");
        free(pPhysicalDevices);
        return -11;
    }

    free(pPhysicalDevices);

    return 1;
}

int allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
    context.vk.device = NULL;

    float normal_priority = 1.0f;
    VkResult result;
    VkBool32 surfaceSupported;
    const unsigned GRAPHICS_FAMILY_INDEX = 0;
    const unsigned PRESENT_FAMILY_INDEX  = 1;
    const unsigned TOTAL_FAMILY_INDEXES  = 2;

    VkDeviceQueueCreateInfo deviceQueueCreateInfos[TOTAL_FAMILY_INDEXES];
    memset(deviceQueueCreateInfos, 0, TOTAL_FAMILY_INDEXES * sizeof(VkDeviceQueueCreateInfo));

    for(int i = 0; i < TOTAL_FAMILY_INDEXES; i++) {
        deviceQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfos[i].pNext = NULL;
        deviceQueueCreateInfos[i].pQueuePriorities = &normal_priority;
        deviceQueueCreateInfos[i].queueCount = 1;
    }

    for(Uint32 p = context.vk.queueFamilyPropertyCount; p != 0; p--) {
        if( (context.vk.pQueueFamilyProperties[p - 1].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
            deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex = p - 1;
        }

        result = vkGetPhysicalDeviceSurfaceSupportKHR(context.vk.physicalDevice, p - 1, context.vk.surface, &surfaceSupported);

        if(result != VK_SUCCESS) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "At index %i for vkGetPhysicalDeviceSurfaceSupportKHR returned %i", p - 1, result);
        }

        if(surfaceSupported == VK_TRUE)
            deviceQueueCreateInfos[PRESENT_FAMILY_INDEX].queueFamilyIndex = p - 1;
    }

    SDL_Log( "Queue Family index %i selected for GRAPHICS_FAMILY_INDEX", deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex);
    SDL_Log( "Queue Family index %i selected for PRESENT_FAMILY_INDEX",  deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex);

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    memset(&physicalDeviceFeatures, 0, sizeof(physicalDeviceFeatures));

    VkDeviceCreateInfo deviceCreateInfo;
    memset(&deviceCreateInfo, 0, sizeof(deviceCreateInfo));
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;

    deviceCreateInfo.pQueueCreateInfos    = deviceQueueCreateInfos;

    if( deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex == deviceQueueCreateInfos[PRESENT_FAMILY_INDEX].queueFamilyIndex)
        deviceCreateInfo.queueCreateInfoCount = 1;
    else
        deviceCreateInfo.queueCreateInfoCount = TOTAL_FAMILY_INDEXES;

    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    deviceCreateInfo.ppEnabledExtensionNames = ppRequiredExtensions;
    deviceCreateInfo.enabledExtensionCount = requiredExtensionsAmount;

    deviceCreateInfo.enabledLayerCount = 0;

    result = vkCreateDevice(context.vk.physicalDevice, &deviceCreateInfo, NULL, &context.vk.device);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %i", result);
        return -9;
    }

    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.graphicsQueue);
    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.presentationQueue);

    return 1;
}

int allocateSwapChain() {
    int foundPriority;
    int currentPriority;

    updateSwapChainCapabilities(context.vk.physicalDevice, context.vk.surface);

    // Find VkSurfaceFormatKHR
    const VkFormat format[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8_SRGB, VK_FORMAT_R8G8B8_SRGB};
    const unsigned FORMAT_AMOUNT = sizeof(format) / sizeof(VkFormat);

    context.vk.surfaceFormat = context.vk.pSurfaceFormat[0];

    foundPriority = FORMAT_AMOUNT;
    currentPriority = FORMAT_AMOUNT;

    for(Uint32 f = context.vk.surfaceFormatCount; f != 0; f--) {
        if(context.vk.pSurfaceFormat[f - 1].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            currentPriority = FORMAT_AMOUNT;

            for(Uint32 p = FORMAT_AMOUNT; p != 0; p--) {
                if(format[p - 1] == context.vk.pSurfaceFormat[f - 1].format) {
                    currentPriority = p - 1;
                    break;
                }
            }

            if(foundPriority > currentPriority) {
                context.vk.surfaceFormat = context.vk.pSurfaceFormat[f - 1];
                foundPriority = currentPriority;
            }
        }
    }

    // Find VkPresentModeKHR
    const VkPresentModeKHR presentModes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR};
    const unsigned PRESENT_MODE_AMOUNT = sizeof(presentModes) / sizeof(VkPresentModeKHR);

    context.vk.presentMode = context.vk.pPresentMode[0];

    foundPriority = PRESENT_MODE_AMOUNT;
    currentPriority = PRESENT_MODE_AMOUNT;

    for(Uint32 f = context.vk.presentModeCount; f != 0; f--) {
        currentPriority = PRESENT_MODE_AMOUNT;

        for(Uint32 p = PRESENT_MODE_AMOUNT; p != 0; p--) {
            if(presentModes[p - 1] == context.vk.pPresentMode[f - 1]) {
                currentPriority = p - 1;
                break;
            }
        }

        if(foundPriority > currentPriority) {
            context.vk.presentMode = context.vk.pPresentMode[f - 1];
            foundPriority = currentPriority;
        }
    }

    // Find VkExtent2D
    const Uint32 MAX_U32 = (Uint32) - 1;

    if(context.vk.surfaceCapabilities.currentExtent.width != MAX_U32)
        context.vk.swapExtent.width = context.vk.surfaceCapabilities.currentExtent.width;
    else {
        int width, height;

        SDL_Vulkan_GetDrawableSize(context.pWindow, &width, &height);

        context.vk.swapExtent.width  = MIN(context.vk.surfaceCapabilities.maxImageExtent.width,  MAX(context.vk.surfaceCapabilities.minImageExtent.width,  (Uint32)width));
        context.vk.swapExtent.height = MIN(context.vk.surfaceCapabilities.maxImageExtent.height, MAX(context.vk.surfaceCapabilities.minImageExtent.height, (Uint32)height));
    }

    return 1;
}

int initVulkan() {
    context.vk.instance = NULL;
    context.vk.physicalDevice = NULL;
    context.vk.pQueueFamilyProperties = NULL;
    context.vk.queueFamilyPropertyCount = 0;
    context.vk.pSurfaceFormat = NULL;
    context.vk.surfaceFormatCount = 0;
    context.vk.pPresentMode = NULL;
    context.vk.presentModeCount = 0;

    int returnCode;

    returnCode = initInstance();
    if( returnCode < 0 )
        return returnCode;

    if(SDL_Vulkan_CreateSurface(context.pWindow, context.vk.instance, &context.vk.surface) != SDL_TRUE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %s", SDL_GetError());
        returnCode = -10;
    }

    const char *const requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    returnCode = findPhysicalDevice(requiredExtensions, 1);
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateLogicalDevice(requiredExtensions, 1);
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateSwapChain();
    if( returnCode < 0 )
        return returnCode;

    return 1;
}

void loop() {
    SDL_Event event;
    int run = 1;

    while(run) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            run = 0;
        }
    }
}

int main(int argc, char **argv) {
    int returnCode;

    if(SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed with %s", SDL_GetError());
        return -1;
    }

    context.pWindow = SDL_CreateWindow(context.title, context.x, context.y, context.w, context.h, context.flags);

    if(context.pWindow == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed with %s", SDL_GetError());
        return -2;
    }

    returnCode = initVulkan();

    if( returnCode < 0 ) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Thus initVulkan() failed with code %s", SDL_GetError());
    }
    else {
        loop();
    }

    if(context.vk.surfaceFormatCount > 0)
        free(context.vk.pSurfaceFormat);

    if(context.vk.presentModeCount > 0)
        free(context.vk.pPresentMode);

    if(context.vk.queueFamilyPropertyCount > 0)
        free(context.vk.pQueueFamilyProperties);

    vkDestroyDevice(context.vk.device, NULL);
    vkDestroySurfaceKHR(context.vk.instance, context.vk.surface, NULL);
    vkDestroyInstance(context.vk.instance, NULL);
    SDL_DestroyWindow(context.pWindow);

    return returnCode;
}
