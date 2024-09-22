#include "v_init.h"

#include "context.h"
#include "u_read.h"

#include <string.h>
#include <assert.h>

#include <vulkan/vulkan.h>
#include "SDL_vulkan.h"
#include "SDL.h"

#include "raymath.h"

typedef struct {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR *pSurfaceFormat;
    Uint32 surfaceFormatCount;
    VkPresentModeKHR *pPresentMode;
    Uint32 presentModeCount;
} SwapChainCapabilities;


static VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, Uint32 *pQueueFamilyPropertyCount);
static VkLayerProperties* allocateLayerPropertiesArray(Uint32 *pPropertyCount);
static int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, Uint32 requiredExtensionCount);
static int querySwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainCapabilities **ppSwapChainCapabilities);
static int initInstance();
static int findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static int allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static int allocateSwapChain();
static int allocateSwapChainImageViews();
static int createRenderPass();
static VkShaderModule allocateShaderModule(Uint8* data, size_t size);
static int allocateGraphicsPipeline();
static int allocateFrameBuffers();
static int allocateCommandPool();
static int createCommandBuffer();
static int allocateSyncObjects();
static void cleanupSwapChain();


int v_init() {
    // Clear the entire vulkan context.
    memset(&context.vk, 0, sizeof(context.vk));

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

    returnCode = createRenderPass();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateGraphicsPipeline();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateCommandPool();
    if( returnCode < 0 )
        return returnCode;

    returnCode = createCommandBuffer();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateSyncObjects();
    if( returnCode < 0 )
        return returnCode;

    return 1;
}

void v_deinit() {
    vkDeviceWaitIdle(context.vk.device);

    cleanupSwapChain();

    if(context.vk.pQueueFamilyProperties != NULL)
        free(context.vk.pQueueFamilyProperties);

    for(Uint32 i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        vkDestroySemaphore(context.vk.device, context.vk.frames[i - 1].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(context.vk.device, context.vk.frames[i - 1].renderFinishedSemaphore, NULL);
        vkDestroyFence(    context.vk.device, context.vk.frames[i - 1].inFlightFence,           NULL);
    }

    vkDestroyCommandPool(context.vk.device, context.vk.commandPool, NULL);
    vkDestroyPipeline(context.vk.device, context.vk.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(context.vk.device, context.vk.pipelineLayout, NULL);
    vkDestroyRenderPass(context.vk.device, context.vk.renderPass, NULL);
    vkDestroyDevice(context.vk.device, NULL);
    vkDestroySurfaceKHR(context.vk.instance, context.vk.surface, NULL);
    vkDestroyInstance(context.vk.instance, NULL);
}

int v_recreate_swap_chain() {
    vkDeviceWaitIdle(context.vk.device);

    int returnCode;

    cleanupSwapChain();

    returnCode = allocateSwapChain();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateSwapChainImageViews();
    if( returnCode < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers();
    if( returnCode < 0 )
        return returnCode;
    return 1;
}


static int allocateSyncObjects() {
    VkResult result;

    VkSemaphoreCreateInfo semaphoreCreateInfo;
    memset(&semaphoreCreateInfo, 0, sizeof(semaphoreCreateInfo));
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo;
    memset(&fenceCreateInfo, 0, sizeof(fenceCreateInfo));
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(Uint32 i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkCreateSemaphore(context.vk.device, &semaphoreCreateInfo, NULL, &context.vk.frames[i - 1].imageAvailableSemaphore);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "imageAvailableSemaphore at index %i creation failed with result: %i", i - 1, result);
            return -33;
        }

        result = vkCreateSemaphore(context.vk.device, &semaphoreCreateInfo, NULL, &context.vk.frames[i - 1].renderFinishedSemaphore);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderFinishedSemaphore at index %i creation failed with result: %i", i - 1, result);
            return -34;
        }

        result = vkCreateFence(context.vk.device, &fenceCreateInfo, NULL, &context.vk.frames[i - 1].inFlightFence);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "inFlightFence at index %i creation failed with result: %i", i - 1, result);
            return -35;
        }
    }

    return 1;
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

static int querySwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainCapabilities **ppSwapChainCapabilities) {
    assert(ppSwapChainCapabilities != NULL);

    VkResult result;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    Uint32 surfaceFormatCount = 0;
    Uint32 presentModeCount = 0;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR had failed with %i", result);
        return -2;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for count had failed with %i", result);
        return -3;
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for count had failed with %i", result);
        return -4;
    }

    SwapChainCapabilities *pSwapChainCapabilities = malloc(
        sizeof(SwapChainCapabilities) +
        surfaceFormatCount * sizeof(VkSurfaceFormatKHR) +
        presentModeCount   * sizeof(VkPresentModeKHR));

    if(pSwapChainCapabilities == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot allocate SwapChainCapabilities!");
        return -5;
    }

    pSwapChainCapabilities->surfaceCapabilities = surfaceCapabilities;
    pSwapChainCapabilities->surfaceFormatCount  = surfaceFormatCount;
    pSwapChainCapabilities->presentModeCount    = presentModeCount;
    pSwapChainCapabilities->pSurfaceFormat      = ((void*)pSwapChainCapabilities) + sizeof(SwapChainCapabilities);
    pSwapChainCapabilities->pPresentMode        = ((void*)&pSwapChainCapabilities->pSurfaceFormat[surfaceFormatCount]);

    assert(pSwapChainCapabilities->pSurfaceFormat != NULL);
    assert(pSwapChainCapabilities->pPresentMode   != NULL);

    *ppSwapChainCapabilities = pSwapChainCapabilities;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &pSwapChainCapabilities->presentModeCount, pSwapChainCapabilities->pPresentMode);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for allocation had failed with %i", result);
        return -6;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSwapChainCapabilities->surfaceFormatCount, pSwapChainCapabilities->pSurfaceFormat);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for allocation had failed with %i", result);
        return -7;
    }

    if(pSwapChainCapabilities->surfaceFormatCount != 0 && pSwapChainCapabilities->presentModeCount != 0) {
        return 1;
    }
    else {
        return 0;
    }
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
                SwapChainCapabilities *pSwapChainCapabilities = NULL;

                if(querySwapChainCapabilities(pPhysicalDevices[i - 1], context.vk.surface, &pSwapChainCapabilities))
                    requiredParameters |= 4;

                if(pSwapChainCapabilities != NULL)
                    free(pSwapChainCapabilities);
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

    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.graphicsQueue);
    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.presentationQueue);

    context.vk.graphicsQueueFamilyIndex     = deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex;
    context.vk.presentationQueueFamilyIndex = deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex;

    return 1;
}

static int allocateSwapChain() {
    SwapChainCapabilities *pSwapChainCapabilities;
    int foundPriority;
    int currentPriority;

    int updateResult = querySwapChainCapabilities(context.vk.physicalDevice, context.vk.surface, &pSwapChainCapabilities);

    if(updateResult < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to update swap chain capabilities %i", updateResult);
        return -16;
    }

    // Find VkSurfaceFormatKHR
    const VkFormat format[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8_SRGB, VK_FORMAT_R8G8B8_SRGB};
    const unsigned FORMAT_AMOUNT = sizeof(format) / sizeof(VkFormat);

    context.vk.surfaceFormat = pSwapChainCapabilities->pSurfaceFormat[0];

    foundPriority = FORMAT_AMOUNT;
    currentPriority = FORMAT_AMOUNT;

    for(Uint32 f = pSwapChainCapabilities->surfaceFormatCount; f != 0; f--) {
        if(pSwapChainCapabilities->pSurfaceFormat[f - 1].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            currentPriority = FORMAT_AMOUNT;

            for(Uint32 p = FORMAT_AMOUNT; p != 0; p--) {
                if(format[p - 1] == pSwapChainCapabilities->pSurfaceFormat[f - 1].format) {
                    currentPriority = p - 1;
                    break;
                }
            }

            if(foundPriority > currentPriority) {
                context.vk.surfaceFormat = pSwapChainCapabilities->pSurfaceFormat[f - 1];
                foundPriority = currentPriority;
            }
        }
    }

    // Find VkPresentModeKHR
    const VkPresentModeKHR presentModes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR};
    const unsigned PRESENT_MODE_AMOUNT = sizeof(presentModes) / sizeof(VkPresentModeKHR);

    context.vk.presentMode = pSwapChainCapabilities->pPresentMode[0];

    foundPriority = PRESENT_MODE_AMOUNT;
    currentPriority = PRESENT_MODE_AMOUNT;

    for(Uint32 f = pSwapChainCapabilities->presentModeCount; f != 0; f--) {
        currentPriority = PRESENT_MODE_AMOUNT;

        for(Uint32 p = PRESENT_MODE_AMOUNT; p != 0; p--) {
            if(presentModes[p - 1] == pSwapChainCapabilities->pPresentMode[f - 1]) {
                currentPriority = p - 1;
                break;
            }
        }

        if(foundPriority > currentPriority) {
            context.vk.presentMode = pSwapChainCapabilities->pPresentMode[f - 1];
            foundPriority = currentPriority;
        }
    }

    // Find VkExtent2D
    const Uint32 MAX_U32 = (Uint32) - 1;

    if(pSwapChainCapabilities->surfaceCapabilities.currentExtent.width != MAX_U32)
        context.vk.swapExtent = pSwapChainCapabilities->surfaceCapabilities.currentExtent;
    else {
        int width, height;

        SDL_Vulkan_GetDrawableSize(context.pWindow, &width, &height);

        context.vk.swapExtent.width  = (Uint32)Clamp( width, pSwapChainCapabilities->surfaceCapabilities.minImageExtent.width,  pSwapChainCapabilities->surfaceCapabilities.maxImageExtent.width);
        context.vk.swapExtent.height = (Uint32)Clamp(height, pSwapChainCapabilities->surfaceCapabilities.minImageExtent.height, pSwapChainCapabilities->surfaceCapabilities.maxImageExtent.height);
    }

    Uint32 imageCount = pSwapChainCapabilities->surfaceCapabilities.minImageCount + 1;

    if(pSwapChainCapabilities->surfaceCapabilities.maxImageCount != 0 && imageCount > pSwapChainCapabilities->surfaceCapabilities.maxImageCount)
        imageCount = pSwapChainCapabilities->surfaceCapabilities.maxImageCount;

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

    swapchainCreateInfo.preTransform = pSwapChainCapabilities->surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = context.vk.presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    free(pSwapChainCapabilities);

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

static int createRenderPass() {
    VkAttachmentDescription colorAttachmentDescription;
    memset(&colorAttachmentDescription, 0, sizeof(colorAttachmentDescription));
    colorAttachmentDescription.format  = context.vk.surfaceFormat.format;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR; // I guess it means clear buffer every frame.
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer.
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference;
    memset(&colorAttachmentReference, 0, sizeof(colorAttachmentReference));
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    memset(&subpassDescription, 0, sizeof(subpassDescription));
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency subpassDependency;
    memset(&subpassDependency, 0, sizeof(subpassDependency));
    subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass    = 0;
    subpassDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo;
    memset(&renderPassCreateInfo, 0, sizeof(renderPassCreateInfo));
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(context.vk.device, &renderPassCreateInfo, NULL, &context.vk.renderPass);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateRenderPass() Failed to allocate %i", result);
        return -21;
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
        return -22;
    }

    Sint64 fragmentShaderCodeLength;
    Uint8* pFragmentShaderCode = u_read_file("hello_world_frag.spv", &fragmentShaderCodeLength);

    if(pFragmentShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load fragment shader code");
        free(pVertexShaderCode);
        return -23;
    }

    VkShaderModule   vertexShaderModule = allocateShaderModule(  pVertexShaderCode,   vertexShaderCodeLength);
    VkShaderModule fragmentShaderModule = allocateShaderModule(pFragmentShaderCode, fragmentShaderCodeLength);

    free(pVertexShaderCode);
    free(pFragmentShaderCode);

    if(vertexShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse vertex shader code!");

        vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

        return -24;
    }
    else if(fragmentShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse fragment shader code!");

        vkDestroyShaderModule(context.vk.device, vertexShaderModule, NULL);

        return -25;
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

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
    memset(&pipelineRasterizationStateCreateInfo, 0, sizeof(pipelineRasterizationStateCreateInfo));
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE; // No shadows for this pipeline.
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Please do not discard everything. I just want to draw a triangle...
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE and VK_POLYGON_MODE_POINT.
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f; // One pixel lines please.
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // OPTIONAL
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f; // OPTIONAL
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; // OPTIONAL

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
    memset(&pipelineMultisampleStateCreateInfo, 0, sizeof(pipelineMultisampleStateCreateInfo));
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // OPTIONAL
    pipelineMultisampleStateCreateInfo.pSampleMask = NULL; // OPTIONAL
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // OPTIONAL
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // OPTIONAL

    // VkPipelineDepthStencilStateCreateInfo // TODO Add that for 3D.

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
    memset(&pipelineColorBlendAttachmentState, 0, sizeof(pipelineColorBlendAttachmentState));
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    memset(&pipelineColorBlendStateCreateInfo, 0, sizeof(pipelineColorBlendStateCreateInfo));
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // OPTIONAL
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // OPTIONAL
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // OPTIONAL
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // OPTIONAL
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // OPTIONAL

    // Here is where uniforms should go.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // OPTIONAL
    pipelineLayoutInfo.pSetLayouts = NULL; // OPTIONAL
    pipelineLayoutInfo.pushConstantRangeCount = 0; // OPTIONAL
    pipelineLayoutInfo.pPushConstantRanges = NULL; // OPTIONAL

    VkResult result = vkCreatePipelineLayout(context.vk.device, &pipelineLayoutInfo, NULL, &context.vk.pipelineLayout);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline Layout creation failed with result: %i", result);

        vkDestroyShaderModule(context.vk.device,   vertexShaderModule, NULL);
        vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

        return -26;
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    memset(&graphicsPipelineCreateInfo, 0, sizeof(graphicsPipelineCreateInfo));

    graphicsPipelineCreateInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = sizeof(pipelineShaderStageCreateInfos) / sizeof(pipelineShaderStageCreateInfos[0]);
    graphicsPipelineCreateInfo.pStages    = pipelineShaderStageCreateInfos;

    graphicsPipelineCreateInfo.pVertexInputState   = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState      = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState   = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState  =  NULL; // OPTIONAL
    graphicsPipelineCreateInfo.pColorBlendState    = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState       = &pipelineDynamicStateInfo;

    graphicsPipelineCreateInfo.layout = context.vk.pipelineLayout;

    graphicsPipelineCreateInfo.renderPass = context.vk.renderPass;
    graphicsPipelineCreateInfo.subpass    = 0;

    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // OPTIONAL
    graphicsPipelineCreateInfo.basePipelineIndex  = -1; // OPTIONAL

    result = vkCreateGraphicsPipelines(context.vk.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &context.vk.graphicsPipeline);

    vkDestroyShaderModule(context.vk.device,   vertexShaderModule, NULL);
    vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateGraphicsPipelines creation failed with result: %i", result);
        return -27;
    }

    return 1;
}

static int allocateFrameBuffers() {
    VkResult result;
    VkFramebufferCreateInfo framebufferCreateInfo;

    context.vk.pSwapChainFramebuffers = calloc(context.vk.swapChainImageCount, sizeof(VkFramebuffer));

    int numberOfFailures = 0;

    for(Uint32 i = context.vk.swapChainImageCount; i != 0; i--) {
        memset(&framebufferCreateInfo, 0, sizeof(framebufferCreateInfo));

        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = context.vk.renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &context.vk.pSwapChainImageViews[i - 1];
        framebufferCreateInfo.width  = context.vk.swapExtent.width;
        framebufferCreateInfo.height = context.vk.swapExtent.height;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(context.vk.device, &framebufferCreateInfo, NULL, &context.vk.pSwapChainFramebuffers[i - 1]);

        if(result != VK_SUCCESS) {
            numberOfFailures++;
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateFramebuffer creation for index %i failed with result: %i", i - 1, result);
        }
    }

    if(numberOfFailures != 0) {
        return -28;
    }
    return 1;
}

static int allocateCommandPool() {
    VkResult result;

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    memset(&commandPoolCreateInfo, 0, sizeof(commandPoolCreateInfo));
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context.vk.graphicsQueueFamilyIndex;

    result = vkCreateCommandPool(context.vk.device, &commandPoolCreateInfo, NULL, &context.vk.commandPool);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCommandPool creation failed with result: %i", result);
        return -29;
    }
    return 1;
}

static int createCommandBuffer() {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    memset(&commandBufferAllocateInfo, 0, sizeof(commandBufferAllocateInfo));
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = context.vk.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    for(Uint32 i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkAllocateCommandBuffers(context.vk.device, &commandBufferAllocateInfo, &context.vk.frames[i - 1].commandBuffer);

        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateCommandBuffers at index %i creation failed with result: %i", i - 1, result);
            return -30;
        }
    }
    return 1;
}

static void cleanupSwapChain() {
    if(context.vk.pSwapChainImages != NULL)
        free(context.vk.pSwapChainImages);

    if(context.vk.pSwapChainImageViews != NULL) {
        for(Uint32 i = context.vk.swapChainImageCount; i != 0; i--) {
            vkDestroyImageView(context.vk.device, context.vk.pSwapChainImageViews[i - 1], NULL);
        }

        free(context.vk.pSwapChainImageViews);
    }

    if(context.vk.pSwapChainFramebuffers != NULL) {
        for(Uint32 i = context.vk.swapChainImageCount; i != 0; i--) {
            vkDestroyFramebuffer(context.vk.device, context.vk.pSwapChainFramebuffers[i - 1], NULL);
        }

        free(context.vk.pSwapChainFramebuffers);
    }

    vkDestroySwapchainKHR(context.vk.device, context.vk.swapChain, NULL);
}
