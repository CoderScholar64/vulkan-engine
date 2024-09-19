#include "v_init.h"

#include "context.h"

#include <vulkan/vulkan.h>
#include "SDL_vulkan.h"
#include "SDL.h"
#include <string.h>
#include "u_math.h"
#include "u_read.h"

static VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, Uint32 *pQueueFamilyPropertyCount);
static VkLayerProperties* allocateLayerPropertiesArray(Uint32 *pPropertyCount);
static int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, Uint32 requiredExtensionCount);
static int updateSwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
static int initInstance();
static int findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static int allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static int allocateSwapChain();
static int allocateSwapChainImageViews();
static VkShaderModule allocateShaderModule(Uint8* data, size_t size);
static int allocateGraphicsPipeline();


int v_init() {
    context.vk.instance = NULL;
    context.vk.physicalDevice = NULL;
    context.vk.pQueueFamilyProperties = NULL;
    context.vk.queueFamilyPropertyCount = 0;

    context.vk.pSurfaceFormat = NULL;
    context.vk.surfaceFormatCount = 0;
    context.vk.pPresentMode = NULL;
    context.vk.presentModeCount = 0;

    context.vk.swapChain = NULL;
    context.vk.swapChainImageCount = 0;
    context.vk.pSwapChainImages = NULL;
    context.vk.pSwapChainImageViews = NULL;

    int returnCode;

    returnCode = initInstance();
    if( returnCode < 0 )
        return returnCode;

    if(SDL_Vulkan_CreateSurface(context.pWindow, context.vk.instance, &context.vk.surface) != SDL_TRUE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %s", SDL_GetError());
        returnCode = -1;
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

    returnCode = allocateSwapChainImageViews();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateGraphicsPipeline();
    if( returnCode < 0 )
        return returnCode;

    return 1;
}

void v_deinit() {
    if(context.vk.pSurfaceFormat != NULL)
        free(context.vk.pSurfaceFormat);

    if(context.vk.pPresentMode != NULL)
        free(context.vk.pPresentMode);

    if(context.vk.pQueueFamilyProperties != NULL)
        free(context.vk.pQueueFamilyProperties);

    if(context.vk.pSwapChainImages != NULL)
        free(context.vk.pSwapChainImages);

    if(context.vk.pSwapChainImageViews != NULL) {
        for(Uint32 i = context.vk.swapChainImageCount; i != 0; i--) {
            vkDestroyImageView(context.vk.device, context.vk.pSwapChainImageViews[i - 1], NULL);
        }

        free(context.vk.pSwapChainImageViews);
    }

    vkDestroySwapchainKHR(context.vk.device, context.vk.swapChain, NULL);
    vkDestroyDevice(context.vk.device, NULL);
    vkDestroySurfaceKHR(context.vk.instance, context.vk.surface, NULL);
    vkDestroyInstance(context.vk.instance, NULL);
}

static VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, Uint32 *pQueueFamilyPropertyCount) {
    *pQueueFamilyPropertyCount = 0;
    VkQueueFamilyProperties *pQueueFamilyProperties = NULL;

    vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, NULL);

    if(*pQueueFamilyPropertyCount != 0)
        pQueueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * (*pQueueFamilyPropertyCount));

    if(pQueueFamilyProperties != NULL)
        vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, pQueueFamilyProperties);

    return pQueueFamilyProperties;
}

static VkLayerProperties* allocateLayerPropertiesArray(Uint32 *pPropertyCount) {
    *pPropertyCount = 0;
    VkLayerProperties *pLayerProperties = NULL;

    vkEnumerateInstanceLayerProperties(pPropertyCount, NULL);

    if(*pPropertyCount != 0)
        pLayerProperties = malloc(sizeof(VkLayerProperties) * (*pPropertyCount));

    if(pLayerProperties != NULL)
        vkEnumerateInstanceLayerProperties(pPropertyCount, pLayerProperties);

    return pLayerProperties;
}

static int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, Uint32 requiredExtensionCount) {
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

static int updateSwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
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
        return -2;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &context.vk.surfaceFormatCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for count had failed with %i", result);
        return -3;
    }

    context.vk.pSurfaceFormat = malloc(context.vk.surfaceFormatCount * sizeof(VkSurfaceFormatKHR));

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &context.vk.surfaceFormatCount, context.vk.pSurfaceFormat);

    if(result != VK_SUCCESS || context.vk.pSurfaceFormat == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for allocation had failed with %i", result);
        return -4;
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &context.vk.presentModeCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for count had failed with %i", result);
        return -5;
    }

    context.vk.pPresentMode = malloc(context.vk.presentModeCount * sizeof(VkPresentModeKHR));

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &context.vk.presentModeCount, context.vk.pPresentMode);

    if(result != VK_SUCCESS || context.vk.pPresentMode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for allocation had failed with %i", result);
        return -6;
    }

    if(context.vk.surfaceFormatCount != 0 && context.vk.presentModeCount != 0)
        return 1;
    else
        return 0;
}

static int initInstance() {
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
        return -7;
    }
    if(extensionCount != 0)
        ppExtensionNames = malloc(sizeof(char*) * extensionCount);
    if(!SDL_Vulkan_GetInstanceExtensions(context.pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting names of extensions had failed with %s", SDL_GetError());
        free(ppExtensionNames);
        return -8;
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
        return -9;
    }

    return 1;
}

static int findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
    Uint32 physicalDevicesCount = 0;
    VkPhysicalDevice *pPhysicalDevices = NULL;

    VkResult result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, NULL);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for amount returned %i", result);
        return -10;
    }

    if(physicalDevicesCount != 0)
        pPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDevicesCount);
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "There are not any rendering device available!");
        return -11;
    }

    if(pPhysicalDevices == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "pPhysicalDevices failed to allocate %i names!", physicalDevicesCount);
        return -12;
    }

    result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, pPhysicalDevices);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for devices returned %i", result);
        free(pPhysicalDevices);
        return -13;
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
        return -14;
    }

    free(pPhysicalDevices);

    return 1;
}

static int allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
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
        return -15;
    }

    //vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.graphicsQueue);

    context.vk.graphicsQueueFamilyIndex     = deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex;
    context.vk.presentationQueueFamilyIndex = deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex;

    return 1;
}

static int allocateSwapChain() {
    int foundPriority;
    int currentPriority;

    int updateResult = updateSwapChainCapabilities(context.vk.physicalDevice, context.vk.surface);

    if(updateResult < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to update swap chain capabilities %i", updateResult);
        return -16;
    }

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
        context.vk.swapExtent = context.vk.surfaceCapabilities.currentExtent;
    else {
        int width, height;

        SDL_Vulkan_GetDrawableSize(context.pWindow, &width, &height);

        context.vk.swapExtent.width  = MIN(context.vk.surfaceCapabilities.maxImageExtent.width,  MAX(context.vk.surfaceCapabilities.minImageExtent.width,  (Uint32)width));
        context.vk.swapExtent.height = MIN(context.vk.surfaceCapabilities.maxImageExtent.height, MAX(context.vk.surfaceCapabilities.minImageExtent.height, (Uint32)height));
    }

    Uint32 imageCount = context.vk.surfaceCapabilities.minImageCount + 1;

    if(context.vk.surfaceCapabilities.maxImageCount != 0 && imageCount > context.vk.surfaceCapabilities.maxImageCount)
        imageCount = context.vk.surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    memset(&swapchainCreateInfo, 0, sizeof(swapchainCreateInfo));
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = context.vk.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = context.vk.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = context.vk.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = context.vk.swapExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const Uint32 familyIndex[] = {context.vk.graphicsQueueFamilyIndex, context.vk.presentationQueueFamilyIndex};
    const unsigned FAMILY_INDEX_AMOUNT = sizeof(familyIndex) / sizeof(familyIndex[0]);

    if(context.vk.graphicsQueueFamilyIndex == context.vk.presentationQueueFamilyIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = NULL;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = FAMILY_INDEX_AMOUNT;
        swapchainCreateInfo.pQueueFamilyIndices = familyIndex;
    }

    swapchainCreateInfo.preTransform = context.vk.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = context.vk.presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(context.vk.device, &swapchainCreateInfo, NULL, &context.vk.swapChain);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swap chain returned %i", result);
        return -17;
    }

    result = vkGetSwapchainImagesKHR(context.vk.device, context.vk.swapChain, &context.vk.swapChainImageCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for count returned %i", result);
        return -18;
    }

    context.vk.pSwapChainImages = malloc(sizeof(VkImage) * context.vk.swapChainImageCount);

    result = vkGetSwapchainImagesKHR(context.vk.device, context.vk.swapChain, &context.vk.swapChainImageCount, context.vk.pSwapChainImages);

    if(result != VK_SUCCESS || context.vk.pSwapChainImages == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for allocation returned %i", result);
        return -19;
    }

    return 1;
}

static int allocateSwapChainImageViews() {
    VkImageViewCreateInfo imageViewCreateInfo;
    VkResult result;

    context.vk.pSwapChainImageViews = malloc(sizeof(VkImageView) * context.vk.swapChainImageCount);
    memset(context.vk.pSwapChainImageViews, 0, sizeof(VkImageView) * context.vk.swapChainImageCount);

    for(Uint32 i = 0; i < context.vk.swapChainImageCount; i++) {
        memset(&imageViewCreateInfo, 0, sizeof(imageViewCreateInfo));
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = context.vk.pSwapChainImages[i];

        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = context.vk.surfaceFormat.format;

        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(context.vk.device, &imageViewCreateInfo, NULL, &context.vk.pSwapChainImageViews[i]);

        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkCreateImageView at index %i for allocate returned %i", i, result);
            return -20;
        }

    }

    return 1;
}

static VkShaderModule allocateShaderModule(Uint8* data, size_t size) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo;
    VkShaderModule shaderModule = NULL;
    VkResult result;

    memset(&shaderModuleCreateInfo, 0, sizeof(shaderModuleCreateInfo));
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = (const Uint32*)(data);

    result = vkCreateShaderModule(context.vk.device, &shaderModuleCreateInfo, NULL, &shaderModule);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader Module Failed to allocate %i", result);

        if(shaderModule != NULL)
            vkDestroyShaderModule(context.vk.device, shaderModule, NULL);

        shaderModule = NULL;
    }

    return shaderModule;
}

static int allocateGraphicsPipeline() {
    Sint64 vertexShaderCodeLength;
    Uint8* pVertexShaderCode = u_read_file("hello_world_vert.spv", &vertexShaderCodeLength);

    if(pVertexShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load vertex shader code");
        return -21;
    }

    Sint64 fragmentShaderCodeLength;
    Uint8* pFragmentShaderCode = u_read_file("hello_world_frag.spv", &fragmentShaderCodeLength);

    if(pFragmentShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load fragment shader code");
        free(pVertexShaderCode);
        return -22;
    }

    VkShaderModule   vertexShaderModule = allocateShaderModule(  pVertexShaderCode,   vertexShaderCodeLength);
    VkShaderModule fragmentShaderModule = allocateShaderModule(pFragmentShaderCode, fragmentShaderCodeLength);

    free(pVertexShaderCode);
    free(pFragmentShaderCode);

    if(vertexShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse vertex shader code!");

        vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

        return -23;
    }
    else if(fragmentShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse fragment shader code!");

        vkDestroyShaderModule(context.vk.device, vertexShaderModule, NULL);

        return -24;
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2];
    const unsigned   VERTEX_INDEX = 0;
    const unsigned FRAGMENT_INDEX = 1;

    memset(&pipelineShaderStageCreateInfos[VERTEX_INDEX], 0, sizeof(VkPipelineShaderStageCreateInfo));
    pipelineShaderStageCreateInfos[VERTEX_INDEX].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].module = vertexShaderModule;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].pName = "main";
    // pipelineShaderStageCreateInfos[VERTEX_INDEX].pSpecializationInfo = NULL; // This allows the specification of constraints

    memset(&pipelineShaderStageCreateInfos[FRAGMENT_INDEX], 0, sizeof(VkPipelineShaderStageCreateInfo));
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].module = fragmentShaderModule;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].pName = "main";
    // pipelineShaderStageCreateInfos[FRAGMENT_INDEX].pSpecializationInfo = NULL; // This allows the specification of constraints

    //TODO Get to the point where vertex data could be added.
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    memset(&pipelineVertexInputStateCreateInfo, 0, sizeof(pipelineVertexInputStateCreateInfo));
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = NULL;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
    memset(&pipelineInputAssemblyStateCreateInfo, 0, sizeof(pipelineInputAssemblyStateCreateInfo));
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float) context.vk.swapExtent.width;
    viewport.height = (float) context.vk.swapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent = context.vk.swapExtent;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo;
    memset(&pipelineDynamicStateInfo, 0, sizeof(pipelineDynamicStateInfo));
    pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    pipelineDynamicStateInfo.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
    memset(&pipelineViewportStateCreateInfo, 0, sizeof(pipelineViewportStateCreateInfo));
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.scissorCount  = 1;

    vkDestroyShaderModule(context.vk.device,   vertexShaderModule, NULL);
    vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

    return 1;
}
