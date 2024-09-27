#include "v_init.h"

#include "context.h"
#include "u_read.h"
#include "v_mem.h"
#include "v_results.h"

#include <assert.h>
#include <string.h>

#include "raymath.h"
#include "SDL_vulkan.h"
#include "SDL.h"
#include <vulkan/vulkan.h>

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
static VEngineResult querySwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainCapabilities **ppSwapChainCapabilities);
static VEngineResult initInstance();
static VEngineResult findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static VEngineResult allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount);
static VEngineResult allocateSwapChain();
static VEngineResult allocateSwapChainImageViews();
static VEngineResult createRenderPass();
static VkShaderModule allocateShaderModule(Uint8* data, size_t size);
static VEngineResult allocateDescriptorSetLayout();
static VEngineResult allocateGraphicsPipeline();
static VEngineResult allocateFrameBuffers();
static VEngineResult allocateCommandPool();
static VEngineResult allocateDepthResources();
static VEngineResult allocateTextureImage();
static VEngineResult allocateTextureImageView();
static VEngineResult allocateDefaultTextureSampler();
static VEngineResult createCommandBuffer();
static VEngineResult allocateSyncObjects();
static VEngineResult allocateDescriptorPool();
static VEngineResult allocateDescriptorSets();
static void cleanupSwapChain();


VEngineResult v_init() {
    // Clear the entire vulkan context.
    memset(&context.vk, 0, sizeof(context.vk));

    VEngineResult returnCode;

    returnCode = initInstance();
    if( returnCode.type < 0 )
        return returnCode;

    if(SDL_Vulkan_CreateSurface(context.pWindow, context.vk.instance, &context.vk.surface) != SDL_TRUE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %s", SDL_GetError());
        returnCode.type  = -1;
        returnCode.point =  0;
    }

    const char *const requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    returnCode = findPhysicalDevice(requiredExtensions, 1);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateLogicalDevice(requiredExtensions, 1);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChain();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChainImageViews();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = createRenderPass();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorSetLayout();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateGraphicsPipeline();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateCommandPool();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDepthResources();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateTextureImage();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateTextureImageView();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDefaultTextureSampler();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = createCommandBuffer();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSyncObjects();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = v_alloc_builtin_vertex_buffer();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = v_alloc_builtin_index_buffer();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = v_alloc_builtin_uniform_buffers();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorPool();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorSets();
    if( returnCode.type < 0 )
        return returnCode;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

void v_deinit() {
    vkDeviceWaitIdle(context.vk.device);

    cleanupSwapChain();

    vkDestroySampler(context.vk.device, context.vk.defaultTextureSampler, NULL);
    vkDestroyImageView(context.vk.device, context.vk.textureImageView, NULL);
    vkDestroyImage(context.vk.device, context.vk.textureImage, NULL);
    vkFreeMemory(context.vk.device, context.vk.textureImageMemory, NULL);
    vkDestroyDescriptorPool(context.vk.device, context.vk.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(context.vk.device, context.vk.descriptorSetLayout, NULL);

    if(context.vk.pQueueFamilyProperties != NULL)
        free(context.vk.pQueueFamilyProperties);

    for(Uint32 i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        vkDestroySemaphore(context.vk.device, context.vk.frames[i - 1].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(context.vk.device, context.vk.frames[i - 1].renderFinishedSemaphore, NULL);
        vkDestroyFence(    context.vk.device, context.vk.frames[i - 1].inFlightFence,           NULL);
        vkDestroyBuffer(   context.vk.device, context.vk.frames[i - 1].uniformBuffer,           NULL);
        vkFreeMemory(      context.vk.device, context.vk.frames[i - 1].uniformBufferMemory,     NULL);
    }

    vkDestroyBuffer(context.vk.device, context.vk.vertexBuffer, NULL);
    vkFreeMemory(context.vk.device, context.vk.vertexBufferMemory, NULL);
    vkDestroyBuffer(context.vk.device, context.vk.indexBuffer, NULL);
    vkFreeMemory(context.vk.device, context.vk.indexBufferMemory, NULL);
    vkDestroyCommandPool(context.vk.device, context.vk.commandPool, NULL);
    vkDestroyPipeline(context.vk.device, context.vk.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(context.vk.device, context.vk.pipelineLayout, NULL);
    vkDestroyRenderPass(context.vk.device, context.vk.renderPass, NULL);
    vkDestroyDevice(context.vk.device, NULL);
    vkDestroySurfaceKHR(context.vk.instance, context.vk.surface, NULL);
    vkDestroyInstance(context.vk.instance, NULL);
}

VEngineResult v_recreate_swap_chain() {
    VEngineResult returnCode;

    vkDeviceWaitIdle(context.vk.device);

    cleanupSwapChain();

    returnCode = allocateSwapChain();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChainImageViews();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDepthResources();
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers();
    if( returnCode.type < 0 )
        return returnCode;
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
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
        return -1; // Cannot check requiredExtensions.
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

static VEngineResult querySwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainCapabilities **ppSwapChainCapabilities) {
    assert(ppSwapChainCapabilities != NULL);

    VkResult result;

    SwapChainCapabilities swapChainCapabilities;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainCapabilities.surfaceCapabilities);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR had failed with %i", result);
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 0)
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &swapChainCapabilities.surfaceFormatCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for count had failed with %i", result);
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 1)
    }

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &swapChainCapabilities.presentModeCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for count had failed with %i", result);
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 2)
    }

    SwapChainCapabilities *pSwapChainCapabilities = malloc(
        sizeof(SwapChainCapabilities) +
        swapChainCapabilities.surfaceFormatCount * sizeof(VkSurfaceFormatKHR) +
        swapChainCapabilities.presentModeCount   * sizeof(VkPresentModeKHR));

    if(pSwapChainCapabilities == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot allocate SwapChainCapabilities!");
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 3)
    }

    *pSwapChainCapabilities = swapChainCapabilities;
    pSwapChainCapabilities->pSurfaceFormat = ((void*)pSwapChainCapabilities) + sizeof(SwapChainCapabilities);
    pSwapChainCapabilities->pPresentMode   = ((void*)&pSwapChainCapabilities->pSurfaceFormat[swapChainCapabilities.surfaceFormatCount]);

    assert(pSwapChainCapabilities->pSurfaceFormat != NULL);
    assert(pSwapChainCapabilities->pPresentMode   != NULL);

    *ppSwapChainCapabilities = pSwapChainCapabilities;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &pSwapChainCapabilities->presentModeCount, pSwapChainCapabilities->pPresentMode);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfacePresentModesKHR for allocation had failed with %i", result);
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 4)
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSwapChainCapabilities->surfaceFormatCount, pSwapChainCapabilities->pSurfaceFormat);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkGetPhysicalDeviceSurfaceFormatsKHR for allocation had failed with %i", result);
        RETURN_RESULT_CODE(VE_QUERY_SWAP_CHAIN_FAILURE, 5)
    }

    if(pSwapChainCapabilities->surfaceFormatCount != 0 && pSwapChainCapabilities->presentModeCount != 0) {
        RETURN_RESULT_CODE(VE_SUCCESS, 0)
    }
    else {
        RETURN_RESULT_CODE(VE_NOT_COMPATIBLE, 0)
    }
}

static VEngineResult initInstance() {
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
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 0)
    }
    if(extensionCount != 0)
        ppExtensionNames = malloc(sizeof(char*) * extensionCount);
    if(!SDL_Vulkan_GetInstanceExtensions(context.pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting names of extensions had failed with %s", SDL_GetError());
        free(ppExtensionNames);
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 1)
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
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 2)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult findPhysicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
    Uint32 physicalDevicesCount = 0;
    VkPhysicalDevice *pPhysicalDevices = NULL;

    VkResult result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, NULL);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for amount returned %i", result);
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 0)
    }

    if(physicalDevicesCount != 0)
        pPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDevicesCount);
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "There are not any rendering device available!");
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 1)
    }

    if(pPhysicalDevices == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "pPhysicalDevices failed to allocate %i names!", physicalDevicesCount);
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 2)
    }

    result = vkEnumeratePhysicalDevices(context.vk.instance, &physicalDevicesCount, pPhysicalDevices);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for devices returned %i", result);
        free(pPhysicalDevices);
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 3)
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

                if(querySwapChainCapabilities(pPhysicalDevices[i - 1], context.vk.surface, &pSwapChainCapabilities).type == VE_SUCCESS)
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
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 4)
    }

    free(pPhysicalDevices);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateLogicalDevice(const char * const* ppRequiredExtensions, Uint32 requiredExtensionsAmount) {
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
        RETURN_RESULT_CODE(VE_ALLOC_LOGICAL_DEVICE_FAILURE, 0)
    }

    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.graphicsQueue);
    vkGetDeviceQueue(context.vk.device, deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex, 0, &context.vk.presentationQueue);

    context.vk.graphicsQueueFamilyIndex     = deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex;
    context.vk.presentationQueueFamilyIndex = deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSwapChain() {
    SwapChainCapabilities *pSwapChainCapabilities;
    int foundPriority;
    int currentPriority;

    VEngineResult updateResult = querySwapChainCapabilities(context.vk.physicalDevice, context.vk.surface, &pSwapChainCapabilities);

    if(updateResult.type < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to update swap chain capabilities %i at point %i", updateResult.type, updateResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 0)
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
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 1)
    }

    result = vkGetSwapchainImagesKHR(context.vk.device, context.vk.swapChain, &context.vk.swapChainFrameCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for count returned %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 2)
    }

    context.vk.pSwapChainFrames = calloc(context.vk.swapChainFrameCount, sizeof(context.vk.pSwapChainFrames[0]));

    VkImage swapChainImages[context.vk.swapChainFrameCount];

    result = vkGetSwapchainImagesKHR(context.vk.device, context.vk.swapChain, &context.vk.swapChainFrameCount, swapChainImages);

    if(result != VK_SUCCESS || context.vk.pSwapChainFrames == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for allocation returned %i", result);

        if(context.vk.pSwapChainFrames != NULL)
            free(context.vk.pSwapChainFrames);

        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 3)
    }

    for(Uint32 i = context.vk.swapChainFrameCount; i != 0; i--) {
        context.vk.pSwapChainFrames[i - 1].image = swapChainImages[i - 1];
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSwapChainImageViews() {
    VEngineResult engineResult;

    for(Uint32 i = 0; i < context.vk.swapChainFrameCount; i++) {
        engineResult = v_alloc_image_view(context.vk.pSwapChainFrames[i].image, context.vk.surfaceFormat.format, 0, VK_IMAGE_ASPECT_COLOR_BIT, &context.vk.pSwapChainFrames[i].imageView);

        if(engineResult.type != VE_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_alloc_image_view failed at index %i for allocate returned %i", i, engineResult.point);
            RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_I_V_FAILURE, i)
        }
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult createRenderPass() {
    const VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT};

    context.vk.depthFormat = v_find_supported_format(
        candidates, sizeof(candidates) / sizeof(candidates[0]),
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if(context.vk.depthFormat == VK_FORMAT_UNDEFINED) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_find_supported_format failed to find any valid format");
        RETURN_RESULT_CODE(VE_CREATE_RENDER_PASS_FAILURE, 0)
    }

    VkAttachmentDescription colorAttachmentDescription = {0};
    colorAttachmentDescription.format  = context.vk.surfaceFormat.format;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR; // I guess it means clear buffer every frame.
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer.
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachmentDescription = {0};
    depthAttachmentDescription.format  = context.vk.depthFormat;
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // I guess it means clear buffer every frame.
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer.
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentReference;
    memset(&colorAttachmentReference, 0, sizeof(colorAttachmentReference));
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {0};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    memset(&subpassDescription, 0, sizeof(subpassDescription));
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency subpassDependency;
    memset(&subpassDependency, 0, sizeof(subpassDependency));
    subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass    = 0;
    subpassDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachmentDescriptions[] = {colorAttachmentDescription, depthAttachmentDescription};

    VkRenderPassCreateInfo renderPassCreateInfo;
    memset(&renderPassCreateInfo, 0, sizeof(renderPassCreateInfo));
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = sizeof(attachmentDescriptions) / sizeof(attachmentDescriptions[0]);
    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(context.vk.device, &renderPassCreateInfo, NULL, &context.vk.renderPass);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateRenderPass() Failed to allocate %i", result);
        RETURN_RESULT_CODE(VE_CREATE_RENDER_PASS_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorSetLayout() {
    VkResult result;

    VkDescriptorSetLayoutBinding descriptorSetBindings[2];
    descriptorSetBindings[0].binding = 0;
    descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetBindings[0].descriptorCount = 1;
    descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetBindings[0].pImmutableSamplers = NULL;

    descriptorSetBindings[1].binding = 1;
    descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetBindings[1].descriptorCount = 1;
    descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorSetBindings[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = 0;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = sizeof(descriptorSetBindings) / sizeof(descriptorSetBindings[0]);
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings;

    result = vkCreateDescriptorSetLayout(context.vk.device, &descriptorSetLayoutCreateInfo, NULL, &context.vk.descriptorSetLayout);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateRenderPass() Failed to allocate %i", result);
        RETURN_RESULT_CODE(VE_DESCRIPTOR_SET_LAYOUT_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
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

static VEngineResult allocateGraphicsPipeline() {
    Sint64 vertexShaderCodeLength;
    Uint8* pVertexShaderCode = u_read_file("hello_world_vert.spv", &vertexShaderCodeLength);

    if(pVertexShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load vertex shader code");
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 0)
    }

    Sint64 fragmentShaderCodeLength;
    Uint8* pFragmentShaderCode = u_read_file("hello_world_frag.spv", &fragmentShaderCodeLength);

    if(pFragmentShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load fragment shader code");
        free(pVertexShaderCode);
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 1)
    }

    VkShaderModule   vertexShaderModule = allocateShaderModule(  pVertexShaderCode,   vertexShaderCodeLength);
    VkShaderModule fragmentShaderModule = allocateShaderModule(pFragmentShaderCode, fragmentShaderCodeLength);

    free(pVertexShaderCode);
    free(pFragmentShaderCode);

    if(vertexShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse vertex shader code!");

        vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 2)
    }
    else if(fragmentShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse fragment shader code!");

        vkDestroyShaderModule(context.vk.device, vertexShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 3)
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
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = sizeof(vertexInputAttributeDescriptions) / sizeof(vertexInputAttributeDescriptions[0]);
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;

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
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Please do not discard everything.
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE and VK_POLYGON_MODE_POINT.
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f; // One pixel lines please.
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {0};
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
    pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    // pipelineDepthStencilStateCreateInfo.front = {}; // Optional
    // pipelineDepthStencilStateCreateInfo.back = {};  // Optional

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
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &context.vk.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // OPTIONAL
    pipelineLayoutInfo.pPushConstantRanges = NULL; // OPTIONAL

    VkResult result = vkCreatePipelineLayout(context.vk.device, &pipelineLayoutInfo, NULL, &context.vk.pipelineLayout);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline Layout creation failed with result: %i", result);

        vkDestroyShaderModule(context.vk.device,   vertexShaderModule, NULL);
        vkDestroyShaderModule(context.vk.device, fragmentShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 4)
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
    graphicsPipelineCreateInfo.pDepthStencilState  = &pipelineDepthStencilStateCreateInfo; // OPTIONAL
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
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 5)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateFrameBuffers() {
    VkResult result;
    VkFramebufferCreateInfo framebufferCreateInfo;

    VkImageView imageViews[2];

    imageViews[1] = context.vk.depthImageView;

    int numberOfFailures = 0;

    for(Uint32 i = context.vk.swapChainFrameCount; i != 0; i--) {
        memset(&framebufferCreateInfo, 0, sizeof(framebufferCreateInfo));

        imageViews[0] = context.vk.pSwapChainFrames[i - 1].imageView;

        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = context.vk.renderPass;
        framebufferCreateInfo.attachmentCount = sizeof(imageViews) / sizeof(imageViews[0]);
        framebufferCreateInfo.pAttachments = imageViews;
        framebufferCreateInfo.width  = context.vk.swapExtent.width;
        framebufferCreateInfo.height = context.vk.swapExtent.height;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(context.vk.device, &framebufferCreateInfo, NULL, &context.vk.pSwapChainFrames[i - 1].framebuffer);

        if(result != VK_SUCCESS) {
            numberOfFailures++;
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateFramebuffer creation for index %i failed with result: %i", i - 1, result);
        }
    }

    if(numberOfFailures != 0) {
        RETURN_RESULT_CODE(VE_ALLOC_FRAME_BUFFERS_FAILURE, numberOfFailures)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateCommandPool() {
    VkResult result;

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    memset(&commandPoolCreateInfo, 0, sizeof(commandPoolCreateInfo));
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context.vk.graphicsQueueFamilyIndex;

    result = vkCreateCommandPool(context.vk.device, &commandPoolCreateInfo, NULL, &context.vk.commandPool);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCommandPool creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_COMMAND_POOL_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDepthResources() {
    VEngineResult engineResult;

    engineResult = v_alloc_image(
        context.vk.swapExtent.width, context.vk.swapExtent.height,
        context.vk.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &context.vk.depthImage, &context.vk.depthImageMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_alloc_image creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 0)
    }

    engineResult = v_alloc_image_view(context.vk.depthImage, context.vk.depthFormat, 0, VK_IMAGE_ASPECT_DEPTH_BIT, &context.vk.depthImageView);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_alloc_image_view creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 1)
    }

    engineResult = v_transition_image_layout(context.vk.depthImage, context.vk.depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_transition_image_layout creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 2)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateTextureImage() {
    qoi_desc QOIdescription;
    const char FILENAME[] = "test_texture.qoi";

    void *pPixels = u_qoi_read(FILENAME, &QOIdescription, 4);

    VkDeviceSize imageSize = 4 * QOIdescription.width * QOIdescription.height;

    if(pPixels == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not read file with name \"%s\"", FILENAME);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 0)
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VEngineResult engineResult = v_alloc_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to make staging buffer for allocateTextureImage");
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 1)
    }

    void* data;
    vkMapMemory(context.vk.device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pPixels, (size_t)imageSize);
    vkUnmapMemory(context.vk.device, stagingBufferMemory);

    free(pPixels);

    engineResult = v_alloc_image(QOIdescription.width, QOIdescription.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &context.vk.textureImage, &context.vk.textureImageMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_alloc_image had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 2)
    }

    engineResult = v_transition_image_layout(context.vk.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_transition_image_layout had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 3)
    }

    engineResult = v_copy_buffer_to_image(stagingBuffer, context.vk.textureImage, QOIdescription.width, QOIdescription.height);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_copy_buffer_to_image had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 4)
    }

    engineResult = v_transition_image_layout(context.vk.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_transition_image_layout had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 5)
    }

    vkDestroyBuffer(context.vk.device, stagingBuffer, NULL);
    vkFreeMemory(context.vk.device, stagingBufferMemory, NULL);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateTextureImageView() {
    VEngineResult engineResult = v_alloc_image_view(context.vk.textureImage, VK_FORMAT_R8G8B8A8_SRGB, 0, VK_IMAGE_ASPECT_COLOR_BIT, &context.vk.textureImageView);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_alloc_image_view failed for allocate returned %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_I_V_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDefaultTextureSampler() {
    VkSamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = NULL;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 1.0;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VkResult result = vkCreateSampler(context.vk.device, &samplerCreateInfo, NULL, &context.vk.defaultTextureSampler);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateSampler creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_DEFAULT_SAMPLER_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult createCommandBuffer() {
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
            RETURN_RESULT_CODE(VE_CREATE_COMMAND_BUFFER_FAILURE, i - 1)
        }
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSyncObjects() {
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
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 0)
        }

        result = vkCreateSemaphore(context.vk.device, &semaphoreCreateInfo, NULL, &context.vk.frames[i - 1].renderFinishedSemaphore);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderFinishedSemaphore at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 1)
        }

        result = vkCreateFence(context.vk.device, &fenceCreateInfo, NULL, &context.vk.frames[i - 1].inFlightFence);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "inFlightFence at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 2)
        }
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorPool() {
    VkResult result;

    VkDescriptorPoolSize descriptorPoolSizes[2];
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    memset(&descriptorPoolCreateInfo, 0, sizeof(descriptorPoolCreateInfo));
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(descriptorPoolSizes[0]);
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    result = vkCreateDescriptorPool(context.vk.device, &descriptorPoolCreateInfo, NULL, &context.vk.descriptorPool);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateDescriptorPool failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_DESCRIPTOR_POOL_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorSets() {
    VkResult result;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = NULL;
    descriptorSetAllocateInfo.descriptorPool = context.vk.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &context.vk.descriptorSetLayout;

    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = context.vk.defaultTextureSampler;
    descriptorImageInfo.imageView = context.vk.textureImageView;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeDescriptorSets[2];
    memset(writeDescriptorSets, 0, sizeof(writeDescriptorSets));
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].pImageInfo = &descriptorImageInfo;

    for(Uint32 i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkAllocateDescriptorSets(context.vk.device, &descriptorSetAllocateInfo, &context.vk.frames[i - 1].descriptorSet);

        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateDescriptorSets at index %i failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_DESCRIPTOR_SET_FAILURE, i - 1)
        }

        descriptorBufferInfo.buffer = context.vk.frames[i - 1].uniformBuffer;

        writeDescriptorSets[0].dstSet = context.vk.frames[i - 1].descriptorSet;
        writeDescriptorSets[1].dstSet = context.vk.frames[i - 1].descriptorSet;

        vkUpdateDescriptorSets(context.vk.device, 2, writeDescriptorSets, 0, NULL);
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static void cleanupSwapChain() {
    vkDestroyImageView( context.vk.device, context.vk.depthImageView,   NULL);
    vkDestroyImage(     context.vk.device, context.vk.depthImage,       NULL);
    vkFreeMemory(       context.vk.device, context.vk.depthImageMemory, NULL);

    for(Uint32 i = context.vk.swapChainFrameCount; i != 0; i--) {
        vkDestroyImageView(  context.vk.device, context.vk.pSwapChainFrames[i - 1].imageView,   NULL);
        vkDestroyFramebuffer(context.vk.device, context.vk.pSwapChainFrames[i - 1].framebuffer, NULL);
    }

    if(context.vk.pSwapChainFrames != NULL)
        free(context.vk.pSwapChainFrames);
    context.vk.pSwapChainFrames = NULL;

    vkDestroySwapchainKHR(context.vk.device, context.vk.swapChain, NULL);
}
