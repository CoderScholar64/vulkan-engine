#include "v_init.h"

#include "context.h"
#include "u_config.h"
#include "u_read.h"
#include "u_maze.h"
#include "u_vector.h"
#include "v_buffer.h"
#include "v_model.h"
#include "v_render.h"
#include "v_results.h"
#include "v_raymath.h"

#include <assert.h>
#include <string.h>

#include "SDL_vulkan.h"
#include "SDL.h"
#include <vulkan/vulkan.h>

typedef struct {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR *pSurfaceFormat;
    uint32_t surfaceFormatCount;
    VkPresentModeKHR *pPresentMode;
    uint32_t presentModeCount;
} SwapChainCapabilities;

static VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, uint32_t *pQueueFamilyPropertyCount);
static VkLayerProperties* allocateLayerPropertiesArray(uint32_t *pPropertyCount);
static int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, uint32_t requiredExtensionCount);
static VEngineResult querySwapChainCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainCapabilities **ppSwapChainCapabilities);
static VEngineResult initInstance(Context *this);
static VEngineResult findPhysicalDevice(Context *this, const char * const* ppRequiredExtensions, uint32_t requiredExtensionsAmount);
static VEngineResult allocateLogicalDevice(Context *this, const char * const* ppRequiredExtensions, uint32_t requiredExtensionsAmount);
static VEngineResult allocateSwapChain(Context *this);
static VEngineResult allocateSwapChainImageViews(Context *this);
static VEngineResult createRenderPass(Context *this);
static VkShaderModule allocateShaderModule(Context *this, uint8_t* data, size_t size);
static VEngineResult allocateDescriptorSetLayout(Context *this);
static VEngineResult allocateGraphicsPipeline(Context *this);
static VEngineResult allocateFrameBuffers(Context *this);
static VEngineResult allocateCommandPool(Context *this);
static VEngineResult allocateColorResources(Context *this);
static VEngineResult allocateDepthResources(Context *this);
static VEngineResult allocateTextureImage(Context *this);
static VEngineResult allocateTextureImageView(Context *this);
static VEngineResult allocateDefaultTextureSampler(Context *this);
static VEngineResult createCommandBuffer(Context *this);
static VEngineResult allocateSyncObjects(Context *this);
static VEngineResult allocateDescriptorPool(Context *this);
static VEngineResult allocateDescriptorSets(Context *this);
static void cleanupSwapChain(Context *this);


VEngineResult v_init_alloc(Context *this) {
    // Clear the entire vulkan context.
    memset(&this->vk, 0, sizeof(this->vk));

    VEngineResult returnCode;

    returnCode = initInstance(this);
    if( returnCode.type < 0 )
        return returnCode;

    if(SDL_Vulkan_CreateSurface(this->pWindow, this->vk.instance, &this->vk.surface) != SDL_TRUE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %s", SDL_GetError());
        returnCode.type  = -1;
        returnCode.point =  0;
    }

    const char *const requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    returnCode = findPhysicalDevice(this, requiredExtensions, 1);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateLogicalDevice(this, requiredExtensions, 1);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChain(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChainImageViews(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = createRenderPass(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorSetLayout(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateGraphicsPipeline(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateCommandPool(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateColorResources(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDepthResources(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateTextureImage(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateTextureImageView(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDefaultTextureSampler(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = createCommandBuffer(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSyncObjects(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = v_model_load(this, "model.glb", &this->vk.modelAmount, &this->vk.pModels);
    if( returnCode.type < 0 )
        return returnCode;

    VModelData *pMazeIndexes[16] = { NULL };

    for(uint32_t i = 0; i < this->vk.modelAmount; i++) {
        size_t lengthOfName = strlen(this->vk.pModels[i].name);

        if(lengthOfName != 4)
            continue;

        unsigned decodedIndex = 0;

        for(unsigned b = 0; b < 4; b++) {
            if(this->vk.pModels[i].name[b] == 'C')
                decodedIndex |= 1 << (3 - b);
        }

        pMazeIndexes[decodedIndex] = &this->vk.pModels[i];
        printf("Model name = %s. Length = %zu decodedIndex = %i\n", this->vk.pModels[i].name, lengthOfName, decodedIndex);
    }

    UMazeData mazeData = {0};
    u_maze_gen_full_sq_grid(&mazeData, 4, 5);
    UMazeGenResult mazeGenResult = {0};
    u_maze_gen(&mazeGenResult, &mazeData, 555, 1);

    size_t mazePieceAmounts[16] = { 0 };

    for(size_t v = 0; v < mazeGenResult.vertexMazeData.vertexAmount; v++) {
        UMazeVertex *pVertex = &mazeGenResult.vertexMazeData.pVertices[v];

        unsigned bitfield = 0;

        for(size_t c = 0; c < mazeGenResult.vertexMazeData.pVertices[v].linkAmount; c++) {
            if(pVertex->metadata.position.x < mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.x)
                bitfield |= 0b1000;
            else
            if(pVertex->metadata.position.x > mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.x)
                bitfield |= 0b0100;

            if(pVertex->metadata.position.y < mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.y)
                bitfield |= 0b0010;
            else
            if(pVertex->metadata.position.y > mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.y)
                bitfield |= 0b0001;
        }

        bitfield = bitfield ^ 0b1111;

        mazePieceAmounts[bitfield]++;

        printf("V: %zu B: 0x%x\n", v, bitfield);
    }

    this->vk.modelArrayAmount = (sizeof(mazePieceAmounts) / sizeof(mazePieceAmounts[0]));

    this->vk.pVModelArray = malloc( sizeof(VModelArray) * (sizeof(mazePieceAmounts) / sizeof(mazePieceAmounts[0])) );

    for(unsigned i = 0; i < sizeof(mazePieceAmounts) / sizeof(mazePieceAmounts[0]); i++) {
        this->vk.pVModelArray[i].pModelData = pMazeIndexes[i];
        this->vk.pVModelArray[i].instanceVector = u_vector_alloc(sizeof(VBufferPushConstantObject), mazePieceAmounts[i]);

        mazePieceAmounts[i] = 0;
    }

    for(size_t v = 0; v < mazeGenResult.vertexMazeData.vertexAmount; v++) {
        UMazeVertex *pVertex = &mazeGenResult.vertexMazeData.pVertices[v];

        unsigned bitfield = 0;

        for(size_t c = 0; c < mazeGenResult.vertexMazeData.pVertices[v].linkAmount; c++) {
            if(pVertex->metadata.position.x < mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.x)
                bitfield |= 0b1000;
            else
            if(pVertex->metadata.position.x > mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.x)
                bitfield |= 0b0100;

            if(pVertex->metadata.position.y < mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.y)
                bitfield |= 0b0010;
            else
            if(pVertex->metadata.position.y > mazeGenResult.vertexMazeData.pVertices[pVertex->pVertexLinkIndexes[c]].metadata.position.y)
                bitfield |= 0b0001;
        }
        bitfield = bitfield ^ 0b1111;

        VBufferPushConstantObject *pPushConst = this->vk.pVModelArray[bitfield].instanceVector.pBuffer;
        pPushConst[mazePieceAmounts[bitfield]].matrix = MatrixTranslate(2 * pVertex->metadata.position.x, 2 * pVertex->metadata.position.y, -3);

        mazePieceAmounts[bitfield]++;
    }

    u_maze_delete_result(&mazeGenResult);
    u_maze_delete_data(&mazeData);

    returnCode = v_buffer_alloc_builtin_uniform(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorPool(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDescriptorSets(this);
    if( returnCode.type < 0 )
        return returnCode;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

void v_init_dealloc(Context *this) {
    vkDeviceWaitIdle(this->vk.device);

    cleanupSwapChain(this);

    vkDestroySampler(this->vk.device, this->vk.defaultTextureSampler, NULL);
    vkDestroyImageView(this->vk.device, this->vk.texture.imageView, NULL);
    vkDestroyImage(this->vk.device, this->vk.texture.image, NULL);
    vkFreeMemory(this->vk.device, this->vk.texture.imageMemory, NULL);
    vkDestroyDescriptorPool(this->vk.device, this->vk.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(this->vk.device, this->vk.descriptorSetLayout, NULL);

    if(this->vk.pQueueFamilyProperties != NULL)
        free(this->vk.pQueueFamilyProperties);

    for(uint32_t i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        vkDestroySemaphore(this->vk.device, this->vk.frames[i - 1].imageAvailableSemaphore, NULL);
        vkDestroySemaphore(this->vk.device, this->vk.frames[i - 1].renderFinishedSemaphore, NULL);
        vkDestroyFence(    this->vk.device, this->vk.frames[i - 1].inFlightFence,           NULL);
        vkDestroyBuffer(   this->vk.device, this->vk.frames[i - 1].uniformBuffer,           NULL);
        vkFreeMemory(      this->vk.device, this->vk.frames[i - 1].uniformBufferMemory,     NULL);
    }

    if(this->vk.pModels != NULL) {
        for(unsigned i = 0; i < this->vk.modelAmount; i++) {
            vkDestroyBuffer(this->vk.device, this->vk.pModels[i].buffer, NULL);
            vkFreeMemory(this->vk.device, this->vk.pModels[i].bufferMemory, NULL);
        }
        free(this->vk.pModels);
    }
    if(this->vk.pVModelArray != NULL) {
        for(unsigned i = 0; i < this->vk.modelArrayAmount; i++) {
            u_vector_free(&this->vk.pVModelArray[i].instanceVector);
        }
        free(this->vk.pVModelArray);
    }
    vkDestroyCommandPool(this->vk.device, this->vk.commandPool, NULL);
    vkDestroyPipeline(this->vk.device, this->vk.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(this->vk.device, this->vk.pipelineLayout, NULL);
    vkDestroyRenderPass(this->vk.device, this->vk.renderPass, NULL);
    vkDestroyDevice(this->vk.device, NULL);
    vkDestroySurfaceKHR(this->vk.instance, this->vk.surface, NULL);
    vkDestroyInstance(this->vk.instance, NULL);
}

VEngineResult v_init_recreate_swap_chain(Context *this) {
    VEngineResult returnCode;

    vkDeviceWaitIdle(this->vk.device);

    cleanupSwapChain(this);

    returnCode = allocateSwapChain(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateSwapChainImageViews(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateColorResources(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateDepthResources(this);
    if( returnCode.type < 0 )
        return returnCode;

    returnCode = allocateFrameBuffers(this);
    if( returnCode.type < 0 )
        return returnCode;
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, uint32_t *pQueueFamilyPropertyCount) {
    *pQueueFamilyPropertyCount = 0;
    VkQueueFamilyProperties *pQueueFamilyProperties = NULL;

    vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, NULL);

    if(*pQueueFamilyPropertyCount != 0)
        pQueueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * (*pQueueFamilyPropertyCount));

    if(pQueueFamilyProperties != NULL)
        vkGetPhysicalDeviceQueueFamilyProperties(device, pQueueFamilyPropertyCount, pQueueFamilyProperties);

    return pQueueFamilyProperties;
}

static VkLayerProperties* allocateLayerPropertiesArray(uint32_t *pPropertyCount) {
    *pPropertyCount = 0;
    VkLayerProperties *pLayerProperties = NULL;

    vkEnumerateInstanceLayerProperties(pPropertyCount, NULL);

    if(*pPropertyCount != 0)
        pLayerProperties = malloc(sizeof(VkLayerProperties) * (*pPropertyCount));

    if(pLayerProperties != NULL)
        vkEnumerateInstanceLayerProperties(pPropertyCount, pLayerProperties);

    return pLayerProperties;
}

static int hasRequiredExtensions(VkPhysicalDevice physicalDevice, const char * const* ppRequiredExtension, uint32_t requiredExtensionCount) {
    uint32_t extensionCount = 0;
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

    for(uint32_t r = 0; r < requiredExtensionCount; r++) {
        found = 0;

        for(uint32_t e = 0; e < extensionCount && found == 0; e++) {
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

static VEngineResult initInstance(Context *this) {
    VkApplicationInfo applicationInfo = {0};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = this->title;
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = NULL;
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    unsigned int extensionCount = 0;
    const char **ppExtensionNames = NULL;
    if(!SDL_Vulkan_GetInstanceExtensions(this->pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting number of extensions had failed with %s", SDL_GetError());
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 0)
    }
    if(extensionCount != 0)
        ppExtensionNames = malloc(sizeof(char*) * extensionCount);
    if(!SDL_Vulkan_GetInstanceExtensions(this->pWindow, &extensionCount, ppExtensionNames)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Getting names of extensions had failed with %s", SDL_GetError());
        free(ppExtensionNames);
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 1)
    }

    for(unsigned int i = 0; i < extensionCount; i++) {
        SDL_Log( "[%i] %s", i, ppExtensionNames[i]);
    }

    uint32_t everyLayerAmount = 0;
    VkLayerProperties* pEveryLayerPropertiesArray = allocateLayerPropertiesArray(&everyLayerAmount);
    const char * name = NULL;
    for(unsigned int i = 0; i < everyLayerAmount; i++) {
        SDL_Log( "[%i] %s", i, pEveryLayerPropertiesArray[i].layerName);

        if(strcmp(pEveryLayerPropertiesArray[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
            name = pEveryLayerPropertiesArray[i].layerName;
    }
    const char ** names = &name;

    VkInstanceCreateInfo instanceCreateInfo = {0};
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

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &this->vk.instance);

    free(pEveryLayerPropertiesArray);

    if(ppExtensionNames != NULL)
        free(ppExtensionNames);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateInstance returned %i", result);
        RETURN_RESULT_CODE(VE_INIT_INSTANCE_FAILURE, 2)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult findPhysicalDevice(Context *this, const char * const* ppRequiredExtensions, uint32_t requiredExtensionsAmount) {
    uint32_t physicalDevicesCount = 0;
    VkPhysicalDevice *pPhysicalDevices = NULL;

    VkResult result = vkEnumeratePhysicalDevices(this->vk.instance, &physicalDevicesCount, NULL);
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

    result = vkEnumeratePhysicalDevices(this->vk.instance, &physicalDevicesCount, pPhysicalDevices);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for devices returned %i", result);
        free(pPhysicalDevices);
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 3)
    }
    u_config_gather_vulkan_devices(&this->config, physicalDevicesCount, pPhysicalDevices);

    VkPhysicalDeviceProperties physicalDeviceProperties;

    uint32_t deviceIndex = physicalDevicesCount;
    uint32_t queueFamilyPropertyCount;
    VkQueueFamilyProperties *pQueueFamilyProperties;
    VkBool32 surfaceSupported;
    int requiredParameters;

    for(uint32_t i = physicalDevicesCount; i != 0; i--) {
        memset(&physicalDeviceProperties, 0, sizeof(physicalDeviceProperties));

        vkGetPhysicalDeviceProperties(pPhysicalDevices[i - 1], &physicalDeviceProperties);

        pQueueFamilyProperties = allocateQueueFamilyArray(pPhysicalDevices[i - 1], &queueFamilyPropertyCount);

        requiredParameters = 0;

        for(uint32_t p = 0; p < queueFamilyPropertyCount; p++) {
            if( (pQueueFamilyProperties[p].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
                requiredParameters |= 1;
            }

            result = vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevices[i - 1], p, this->vk.surface, &surfaceSupported);

            if(result != VK_SUCCESS) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "At index %i for vkGetPhysicalDeviceSurfaceSupportKHR returned %i", p - 1, result);
            }
            else if(surfaceSupported)
                requiredParameters |= 2;

            // Check for required extensions
            if(hasRequiredExtensions(pPhysicalDevices[i - 1], ppRequiredExtensions, requiredExtensionsAmount)) {
                SwapChainCapabilities *pSwapChainCapabilities = NULL;

                if(querySwapChainCapabilities(pPhysicalDevices[i - 1], this->vk.surface, &pSwapChainCapabilities).type == VE_SUCCESS)
                    requiredParameters |= 4;

                if(pSwapChainCapabilities != NULL)
                    free(pSwapChainCapabilities);
            }


        }

        free(pQueueFamilyProperties);

        if(requiredParameters == 7) {
            if(memcmp(this->config.current.graphicsCardPipelineCacheUUID, physicalDeviceProperties.pipelineCacheUUID, VK_UUID_SIZE) == 0) {
                deviceIndex = i - 1;
                break;
            }
            else if(deviceIndex == physicalDevicesCount) {
                deviceIndex = i - 1;
            }
            else if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                deviceIndex = i - 1;
            }
        }
    }
    SDL_Log( "Index %i device selected", deviceIndex);

    memset(&physicalDeviceProperties, 0, sizeof(physicalDeviceProperties));
    vkGetPhysicalDeviceProperties(pPhysicalDevices[deviceIndex], &physicalDeviceProperties);
    for(unsigned i = 0; i < VK_UUID_SIZE; i++) {
        this->config.current.graphicsCardPipelineCacheUUID[i] = physicalDeviceProperties.pipelineCacheUUID[i];
    }

    if(deviceIndex != physicalDevicesCount) {
        this->vk.physicalDevice = pPhysicalDevices[deviceIndex];

        this->vk.pQueueFamilyProperties = allocateQueueFamilyArray(this->vk.physicalDevice, &this->vk.queueFamilyPropertyCount);

        u_config_gather_vulkan_limits(&this->config, this->vk.physicalDevice);

        u_config_bound(&this->config);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find suitable device!");
        free(pPhysicalDevices);
        RETURN_RESULT_CODE(VE_FIND_PHYSICAL_DEVICE_FAILURE, 4)
    }

    this->vk.mmaa.samples = v_buffer_sample_flag_bit(this, this->config.current.sampleCount);
    this->config.current.sampleCount = this->vk.mmaa.samples;

    free(pPhysicalDevices);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateLogicalDevice(Context *this, const char * const* ppRequiredExtensions, uint32_t requiredExtensionsAmount) {
    this->vk.device = NULL;

    float normal_priority = 1.0f;
    VkResult result;
    VkBool32 surfaceSupported;

    VkDeviceQueueCreateInfo deviceQueueCreateInfos[] = { {0}, {0} };
    const unsigned GRAPHICS_FAMILY_INDEX = 0;
    const unsigned PRESENT_FAMILY_INDEX  = 1;
    const unsigned TOTAL_FAMILY_INDEXES  = sizeof(deviceQueueCreateInfos) / sizeof(deviceQueueCreateInfos[0]);

    for(int i = 0; i < TOTAL_FAMILY_INDEXES; i++) {
        deviceQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfos[i].pNext = NULL;
        deviceQueueCreateInfos[i].pQueuePriorities = &normal_priority;
        deviceQueueCreateInfos[i].queueCount = 1;
    }

    for(uint32_t p = this->vk.queueFamilyPropertyCount; p != 0; p--) {
        if( (this->vk.pQueueFamilyProperties[p - 1].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
            deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex = p - 1;
        }

        result = vkGetPhysicalDeviceSurfaceSupportKHR(this->vk.physicalDevice, p - 1, this->vk.surface, &surfaceSupported);

        if(result != VK_SUCCESS) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "At index %i for vkGetPhysicalDeviceSurfaceSupportKHR returned %i", p - 1, result);
        }

        if(surfaceSupported == VK_TRUE)
            deviceQueueCreateInfos[PRESENT_FAMILY_INDEX].queueFamilyIndex = p - 1;
    }

    SDL_Log( "Queue Family index %i selected for GRAPHICS_FAMILY_INDEX", deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex);
    SDL_Log( "Queue Family index %i selected for PRESENT_FAMILY_INDEX",  deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex);

    VkPhysicalDeviceFeatures physicalDeviceFeatures = {0};

    VkDeviceCreateInfo deviceCreateInfo = {0};
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

    result = vkCreateDevice(this->vk.physicalDevice, &deviceCreateInfo, NULL, &this->vk.device);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_LOGICAL_DEVICE_FAILURE, 0)
    }

    vkGetDeviceQueue(this->vk.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &this->vk.graphicsQueue);
    vkGetDeviceQueue(this->vk.device, deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex, 0, &this->vk.presentationQueue);

    this->vk.graphicsQueueFamilyIndex     = deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex;
    this->vk.presentationQueueFamilyIndex = deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex;

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSwapChain(Context *this) {
    SwapChainCapabilities *pSwapChainCapabilities;
    int foundPriority;
    int currentPriority;

    VEngineResult updateResult = querySwapChainCapabilities(this->vk.physicalDevice, this->vk.surface, &pSwapChainCapabilities);

    if(updateResult.type < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to update swap chain capabilities %i at point %i", updateResult.type, updateResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 0)
    }

    // Find VkSurfaceFormatKHR
    const VkFormat format[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8_SRGB, VK_FORMAT_R8G8B8_SRGB};
    const unsigned FORMAT_AMOUNT = sizeof(format) / sizeof(VkFormat);

    this->vk.surfaceFormat = pSwapChainCapabilities->pSurfaceFormat[0];

    foundPriority = FORMAT_AMOUNT;
    currentPriority = FORMAT_AMOUNT;

    for(uint32_t f = pSwapChainCapabilities->surfaceFormatCount; f != 0; f--) {
        if(pSwapChainCapabilities->pSurfaceFormat[f - 1].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            currentPriority = FORMAT_AMOUNT;

            for(uint32_t p = FORMAT_AMOUNT; p != 0; p--) {
                if(format[p - 1] == pSwapChainCapabilities->pSurfaceFormat[f - 1].format) {
                    currentPriority = p - 1;
                    break;
                }
            }

            if(foundPriority > currentPriority) {
                this->vk.surfaceFormat = pSwapChainCapabilities->pSurfaceFormat[f - 1];
                foundPriority = currentPriority;
            }
        }
    }

    // Find VkPresentModeKHR
    const VkPresentModeKHR presentModes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR};
    const unsigned PRESENT_MODE_AMOUNT = sizeof(presentModes) / sizeof(VkPresentModeKHR);

    this->vk.presentMode = pSwapChainCapabilities->pPresentMode[0];

    foundPriority = PRESENT_MODE_AMOUNT;
    currentPriority = PRESENT_MODE_AMOUNT;

    for(uint32_t f = pSwapChainCapabilities->presentModeCount; f != 0; f--) {
        currentPriority = PRESENT_MODE_AMOUNT;

        for(uint32_t p = PRESENT_MODE_AMOUNT; p != 0; p--) {
            if(presentModes[p - 1] == pSwapChainCapabilities->pPresentMode[f - 1]) {
                currentPriority = p - 1;
                break;
            }
        }

        if(foundPriority > currentPriority) {
            this->vk.presentMode = pSwapChainCapabilities->pPresentMode[f - 1];
            foundPriority = currentPriority;
        }
    }

    // Find VkExtent2D
    if(pSwapChainCapabilities->surfaceCapabilities.currentExtent.width != UINT32_MAX)
        this->vk.swapExtent = pSwapChainCapabilities->surfaceCapabilities.currentExtent;
    else {
        int width, height;

        SDL_Vulkan_GetDrawableSize(this->pWindow, &width, &height);

        this->vk.swapExtent.width  = (uint32_t)Clamp( width, pSwapChainCapabilities->surfaceCapabilities.minImageExtent.width,  pSwapChainCapabilities->surfaceCapabilities.maxImageExtent.width);
        this->vk.swapExtent.height = (uint32_t)Clamp(height, pSwapChainCapabilities->surfaceCapabilities.minImageExtent.height, pSwapChainCapabilities->surfaceCapabilities.maxImageExtent.height);
    }

    uint32_t imageCount = pSwapChainCapabilities->surfaceCapabilities.minImageCount + 1;

    if(pSwapChainCapabilities->surfaceCapabilities.maxImageCount != 0 && imageCount > pSwapChainCapabilities->surfaceCapabilities.maxImageCount)
        imageCount = pSwapChainCapabilities->surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = this->vk.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = this->vk.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = this->vk.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = this->vk.swapExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t familyIndex[] = {this->vk.graphicsQueueFamilyIndex, this->vk.presentationQueueFamilyIndex};
    const unsigned FAMILY_INDEX_AMOUNT = sizeof(familyIndex) / sizeof(familyIndex[0]);

    if(this->vk.graphicsQueueFamilyIndex == this->vk.presentationQueueFamilyIndex) {
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
    swapchainCreateInfo.presentMode = this->vk.presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    free(pSwapChainCapabilities);

    VkResult result = vkCreateSwapchainKHR(this->vk.device, &swapchainCreateInfo, NULL, &this->vk.swapChain);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swap chain returned %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 1)
    }

    result = vkGetSwapchainImagesKHR(this->vk.device, this->vk.swapChain, &this->vk.swapChainFrameCount, NULL);

    if(result < VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for count returned %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 2)
    }

    this->vk.pSwapChainFrames = calloc(this->vk.swapChainFrameCount, sizeof(this->vk.pSwapChainFrames[0]));

    VkImage swapChainImages[this->vk.swapChainFrameCount];

    result = vkGetSwapchainImagesKHR(this->vk.device, this->vk.swapChain, &this->vk.swapChainFrameCount, swapChainImages);

    if(result != VK_SUCCESS || this->vk.pSwapChainFrames == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to vkGetSwapchainImagesKHR for allocation returned %i", result);

        if(this->vk.pSwapChainFrames != NULL)
            free(this->vk.pSwapChainFrames);

        RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_FAILURE, 3)
    }

    for(uint32_t i = this->vk.swapChainFrameCount; i != 0; i--) {
        this->vk.pSwapChainFrames[i - 1].image = swapChainImages[i - 1];
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSwapChainImageViews(Context *this) {
    VEngineResult engineResult;

    for(uint32_t i = 0; i < this->vk.swapChainFrameCount; i++) {
        engineResult = v_buffer_alloc_image_view(this, this->vk.pSwapChainFrames[i].image, this->vk.surfaceFormat.format, 0, VK_IMAGE_ASPECT_COLOR_BIT, &this->vk.pSwapChainFrames[i].imageView, 1);

        if(engineResult.type != VE_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image_view failed at index %i for allocate returned %i", i, engineResult.point);
            RETURN_RESULT_CODE(VE_ALLOC_SWAP_CHAIN_I_V_FAILURE, i)
        }
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult createRenderPass(Context *this) {
    const VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT};

    this->vk.depthFormat = v_buffer_find_supported_format(
        this,
        candidates, sizeof(candidates) / sizeof(candidates[0]),
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if(this->vk.depthFormat == VK_FORMAT_UNDEFINED) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_find_supported_format failed to find any valid format");
        RETURN_RESULT_CODE(VE_CREATE_RENDER_PASS_FAILURE, 0)
    }

    VkAttachmentDescription attachmentDescriptions[] = {{0}, {0}, {0}};
    const unsigned COLOR_INDEX = 0;
    const unsigned DEPTH_INDEX = 1;
    const unsigned COLOR_RESOLVE_INDEX = 2;

    attachmentDescriptions[COLOR_INDEX].format  = this->vk.surfaceFormat.format;
    attachmentDescriptions[COLOR_INDEX].samples = this->vk.mmaa.samples;
    attachmentDescriptions[COLOR_INDEX].loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR; // I guess it means clear buffer every frame.
    attachmentDescriptions[COLOR_INDEX].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[COLOR_INDEX].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer.
    attachmentDescriptions[COLOR_INDEX].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[COLOR_INDEX].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if(this->vk.mmaa.samples == VK_SAMPLE_COUNT_1_BIT)
        attachmentDescriptions[COLOR_INDEX].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    else
        attachmentDescriptions[COLOR_INDEX].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachmentDescriptions[DEPTH_INDEX].format  = this->vk.depthFormat;
    attachmentDescriptions[DEPTH_INDEX].samples = this->vk.mmaa.samples;
    attachmentDescriptions[DEPTH_INDEX].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // I guess it means clear buffer every frame.
    attachmentDescriptions[DEPTH_INDEX].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[DEPTH_INDEX].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer.
    attachmentDescriptions[DEPTH_INDEX].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[DEPTH_INDEX].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[DEPTH_INDEX].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if(this->vk.mmaa.samples != VK_SAMPLE_COUNT_1_BIT) {
        attachmentDescriptions[COLOR_RESOLVE_INDEX].format  = this->vk.surfaceFormat.format;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[COLOR_RESOLVE_INDEX].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    VkAttachmentReference colorAttachmentReference = {0};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {0};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorResolveAttachmentReference = {0};
    colorResolveAttachmentReference.attachment = 2;
    colorResolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {0};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

    if(this->vk.mmaa.samples != VK_SAMPLE_COUNT_1_BIT)
        subpassDescription.pResolveAttachments = &colorResolveAttachmentReference;

    VkSubpassDependency subpassDependency = {0};
    subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass    = 0;
    subpassDependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {0};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    if(this->vk.mmaa.samples == VK_SAMPLE_COUNT_1_BIT)
        renderPassCreateInfo.attachmentCount = 2;
    else
        renderPassCreateInfo.attachmentCount = 3;

    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(this->vk.device, &renderPassCreateInfo, NULL, &this->vk.renderPass);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateRenderPass() Failed to allocate %i", result);
        RETURN_RESULT_CODE(VE_CREATE_RENDER_PASS_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorSetLayout(Context *this) {
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

    result = vkCreateDescriptorSetLayout(this->vk.device, &descriptorSetLayoutCreateInfo, NULL, &this->vk.descriptorSetLayout);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "allocateDescriptorSetLayout() Failed to allocate %i", result);
        RETURN_RESULT_CODE(VE_DESCRIPTOR_SET_LAYOUT_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VkShaderModule allocateShaderModule(Context *this, uint8_t* data, size_t size) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {0};
    VkShaderModule shaderModule = NULL;
    VkResult result;

    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = (const uint32_t*)(data);

    result = vkCreateShaderModule(this->vk.device, &shaderModuleCreateInfo, NULL, &shaderModule);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader Module Failed to allocate %i", result);

        if(shaderModule != NULL)
            vkDestroyShaderModule(this->vk.device, shaderModule, NULL);

        shaderModule = NULL;
    }

    return shaderModule;
}

static VEngineResult allocateGraphicsPipeline(Context *this) {
    int64_t vertexShaderCodeLength;
    uint8_t* pVertexShaderCode = u_read_file("hello_world_vert.spv", &vertexShaderCodeLength);

    if(pVertexShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load vertex shader code");
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 0)
    }

    int64_t fragmentShaderCodeLength;
    uint8_t* pFragmentShaderCode = u_read_file("hello_world_frag.spv", &fragmentShaderCodeLength);

    if(pFragmentShaderCode == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load fragment shader code");
        free(pVertexShaderCode);
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 1)
    }

    VkShaderModule   vertexShaderModule = allocateShaderModule(this, pVertexShaderCode,   vertexShaderCodeLength);
    VkShaderModule fragmentShaderModule = allocateShaderModule(this, pFragmentShaderCode, fragmentShaderCodeLength);

    free(pVertexShaderCode);
    free(pFragmentShaderCode);

    if(vertexShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse vertex shader code!");

        vkDestroyShaderModule(this->vk.device, fragmentShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 2)
    }
    else if(fragmentShaderModule == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vulkan failed to parse fragment shader code!");

        vkDestroyShaderModule(this->vk.device, vertexShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 3)
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2] = { {0}, {0} };
    const unsigned   VERTEX_INDEX = 0;
    const unsigned FRAGMENT_INDEX = 1;

    pipelineShaderStageCreateInfos[VERTEX_INDEX].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].module = vertexShaderModule;
    pipelineShaderStageCreateInfos[VERTEX_INDEX].pName = "main";
    // pipelineShaderStageCreateInfos[VERTEX_INDEX].pSpecializationInfo = NULL; // This allows the specification of constraints

    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].module = fragmentShaderModule;
    pipelineShaderStageCreateInfos[FRAGMENT_INDEX].pName = "main";
    // pipelineShaderStageCreateInfos[FRAGMENT_INDEX].pSpecializationInfo = NULL; // This allows the specification of constraints

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {0};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &V_BUFFER_VertexBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = sizeof(V_BUFFER_VertexInputAttributeDescriptions) / sizeof(V_BUFFER_VertexInputAttributeDescriptions[0]);
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = V_BUFFER_VertexInputAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {0};
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float) this->vk.swapExtent.width;
    viewport.height = (float) this->vk.swapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent = this->vk.swapExtent;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo = {0};
    pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    pipelineDynamicStateInfo.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {0};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount =  1;
    pipelineViewportStateCreateInfo.pViewports    = &viewport;
    pipelineViewportStateCreateInfo.scissorCount  =  1;
    pipelineViewportStateCreateInfo.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {0};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE; // No shadows for this pipeline.
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Please do not discard everything.
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // VK_POLYGON_MODE_LINE and VK_POLYGON_MODE_POINT.
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f; // One pixel lines please.
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // OPTIONAL
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f; // OPTIONAL
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; // OPTIONAL

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {0};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = this->vk.mmaa.samples;
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

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {0};
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {0};
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
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &this->vk.descriptorSetLayout;

    VkPushConstantRange pushConstant = {0};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(VBufferPushConstantObject);
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipelineLayoutInfo.pushConstantRangeCount = 1; // OPTIONAL
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant; // OPTIONAL

    VkResult result = vkCreatePipelineLayout(this->vk.device, &pipelineLayoutInfo, NULL, &this->vk.pipelineLayout);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline Layout creation failed with result: %i", result);

        vkDestroyShaderModule(this->vk.device,   vertexShaderModule, NULL);
        vkDestroyShaderModule(this->vk.device, fragmentShaderModule, NULL);

        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 4)
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {0};

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

    graphicsPipelineCreateInfo.layout = this->vk.pipelineLayout;

    graphicsPipelineCreateInfo.renderPass = this->vk.renderPass;
    graphicsPipelineCreateInfo.subpass    = 0;

    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // OPTIONAL
    graphicsPipelineCreateInfo.basePipelineIndex  = -1; // OPTIONAL

    result = vkCreateGraphicsPipelines(this->vk.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &this->vk.graphicsPipeline);

    vkDestroyShaderModule(this->vk.device,   vertexShaderModule, NULL);
    vkDestroyShaderModule(this->vk.device, fragmentShaderModule, NULL);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateGraphicsPipelines creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_GRAPH_PIPELINE_FAILURE, 5)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateFrameBuffers(Context *this) {
    VkResult result;

    VkImageView imageViews[3] = { NULL };

    if(this->vk.mmaa.samples != VK_SAMPLE_COUNT_1_BIT)
        imageViews[0] = this->vk.mmaa.imageView;

    imageViews[1] = this->vk.depthImageView;

    VkFramebufferCreateInfo framebufferCreateInfo = {0};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = this->vk.renderPass;

    framebufferCreateInfo.attachmentCount = sizeof(imageViews) / sizeof(imageViews[0]);
    if(this->vk.mmaa.samples == VK_SAMPLE_COUNT_1_BIT)
        framebufferCreateInfo.attachmentCount--;

    framebufferCreateInfo.pAttachments = imageViews;
    framebufferCreateInfo.width  = this->vk.swapExtent.width;
    framebufferCreateInfo.height = this->vk.swapExtent.height;
    framebufferCreateInfo.layers = 1;

    int numberOfFailures = 0;

    for(uint32_t i = this->vk.swapChainFrameCount; i != 0; i--) {
        if(this->vk.mmaa.samples == VK_SAMPLE_COUNT_1_BIT)
            imageViews[0] = this->vk.pSwapChainFrames[i - 1].imageView;
        else
            imageViews[2] = this->vk.pSwapChainFrames[i - 1].imageView;

        result = vkCreateFramebuffer(this->vk.device, &framebufferCreateInfo, NULL, &this->vk.pSwapChainFrames[i - 1].framebuffer);

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

static VEngineResult allocateCommandPool(Context *this) {
    VkResult result;

    VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = this->vk.graphicsQueueFamilyIndex;

    result = vkCreateCommandPool(this->vk.device, &commandPoolCreateInfo, NULL, &this->vk.commandPool);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCommandPool creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_COMMAND_POOL_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateColorResources(Context *this) {
    VEngineResult engineResult;

    if(this->vk.mmaa.samples == VK_SAMPLE_COUNT_1_BIT)
        RETURN_RESULT_CODE(VE_SUCCESS, 0)

    VkFormat colorFormat = this->vk.surfaceFormat.format;

    engineResult = v_buffer_alloc_image(this, this->vk.swapExtent.width, this->vk.swapExtent.height, 1, this->vk.mmaa.samples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->vk.mmaa.image, &this->vk.mmaa.imageMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_COLOR_BUFFER_FAILURE, 0)
    }

    engineResult = v_buffer_alloc_image_view(this, this->vk.mmaa.image, colorFormat, 0, VK_IMAGE_ASPECT_COLOR_BIT, &this->vk.mmaa.imageView, 1);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image_view creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_COLOR_BUFFER_FAILURE, 1)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 1)
}

static VEngineResult allocateDepthResources(Context *this) {
    VEngineResult engineResult;

    engineResult = v_buffer_alloc_image(
        this,
        this->vk.swapExtent.width, this->vk.swapExtent.height,
        1,
        this->vk.mmaa.samples,
        this->vk.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &this->vk.depthImage, &this->vk.depthImageMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 0)
    }

    engineResult = v_buffer_alloc_image_view(this, this->vk.depthImage, this->vk.depthFormat, 0, VK_IMAGE_ASPECT_DEPTH_BIT, &this->vk.depthImageView, 1);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image_view creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 1)
    }

    engineResult = v_buffer_transition_image_layout(this, this->vk.depthImage, this->vk.depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_transition_image_layout creation failed with result: %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_DEPTH_BUFFER_FAILURE, 2)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static uint32_t getMipLevel(uint32_t width, uint32_t height) {
    uint32_t dimension = height;

    if(width > height)
        dimension =  width;

    for(unsigned i = 0; i < 32; i++) {
        if(dimension <= (1 << i))
            return i + 1;
    }
    return 1;
}

static VEngineResult allocateTextureImage(Context *this) {
    qoi_desc QOIdescription;
    qoi_desc mipQOIdescription;
    const char FILENAME[] = "test_texture_%i.qoi";
    char filename[64];

    snprintf(filename, sizeof(filename) / sizeof(filename[0]), FILENAME, 0);

    void *pPixels = u_read_qoi(filename, &QOIdescription, 4);

    VkDeviceSize firstImageSizeSq = 4 * QOIdescription.width * QOIdescription.height;
    VkDeviceSize mipmapSizeSq = firstImageSizeSq + firstImageSizeSq / 3;

    if(pPixels == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not read file with name \"%s\"", FILENAME);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 0)
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VEngineResult engineResult = v_buffer_alloc(this, mipmapSizeSq, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to make staging buffer for allocateTextureImage");
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 1)
    }

    uint32_t mipLevel = getMipLevel(QOIdescription.width, QOIdescription.height);
    this->vk.texture.mipLevels = mipLevel;

    mipQOIdescription = QOIdescription;

    void* data;
    vkMapMemory(this->vk.device, stagingBufferMemory, 0, mipmapSizeSq, 0, &data);
    for(uint32_t m = 0; m < mipLevel; m++) {
        VkDeviceSize imageSizeSq = 4 * mipQOIdescription.width * mipQOIdescription.height;

        memcpy(data, pPixels, (size_t)imageSizeSq);

        data += imageSizeSq;

        free(pPixels);

        if(m + 1 == mipLevel)
            break;

        snprintf(filename, sizeof(filename) / sizeof(filename[0]), FILENAME, m + 1);

        pPixels = u_read_qoi(filename, &mipQOIdescription, 4);
    }
    vkUnmapMemory(this->vk.device, stagingBufferMemory);

    engineResult = v_buffer_alloc_image(this, QOIdescription.width, QOIdescription.height, this->vk.texture.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->vk.texture.image, &this->vk.texture.imageMemory);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 2)
    }

    engineResult = v_buffer_transition_image_layout(this, this->vk.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->vk.texture.mipLevels);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_transition_image_layout had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 3)
    }

    engineResult = v_buffer_copy_to_image(this, stagingBuffer, this->vk.texture.image, QOIdescription.width, QOIdescription.height, firstImageSizeSq, this->vk.texture.mipLevels);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_copy_to_image had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 4)
    }

    engineResult = v_buffer_transition_image_layout(this, this->vk.texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->vk.texture.mipLevels);
    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_transition_image_layout had failed with %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_IMAGE_FAILURE, 5)
    }

    vkDestroyBuffer(this->vk.device, stagingBuffer, NULL);
    vkFreeMemory(this->vk.device, stagingBufferMemory, NULL);

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateTextureImageView(Context *this) {
    VEngineResult engineResult = v_buffer_alloc_image_view(this, this->vk.texture.image, VK_FORMAT_R8G8B8A8_SRGB, 0, VK_IMAGE_ASPECT_COLOR_BIT, &this->vk.texture.imageView, this->vk.texture.mipLevels);

    if(engineResult.type != VE_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "v_buffer_alloc_image_view failed for allocate returned %i", engineResult.point);
        RETURN_RESULT_CODE(VE_ALLOC_TEXTURE_I_V_FAILURE, 0)
    }

    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDefaultTextureSampler(Context *this) {
    VkSamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = NULL;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
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
    samplerCreateInfo.maxLod = (float)this->vk.texture.mipLevels;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VkResult result = vkCreateSampler(this->vk.device, &samplerCreateInfo, NULL, &this->vk.defaultTextureSampler);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateSampler creation failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_DEFAULT_SAMPLER_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult createCommandBuffer(Context *this) {
    VkResult result;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = this->vk.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    for(uint32_t i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkAllocateCommandBuffers(this->vk.device, &commandBufferAllocateInfo, &this->vk.frames[i - 1].commandBuffer);

        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateCommandBuffers at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_CREATE_COMMAND_BUFFER_FAILURE, i - 1)
        }
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateSyncObjects(Context *this) {
    VkResult result;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {0};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkCreateSemaphore(this->vk.device, &semaphoreCreateInfo, NULL, &this->vk.frames[i - 1].imageAvailableSemaphore);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "imageAvailableSemaphore at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 0)
        }

        result = vkCreateSemaphore(this->vk.device, &semaphoreCreateInfo, NULL, &this->vk.frames[i - 1].renderFinishedSemaphore);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderFinishedSemaphore at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 1)
        }

        result = vkCreateFence(this->vk.device, &fenceCreateInfo, NULL, &this->vk.frames[i - 1].inFlightFence);
        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "inFlightFence at index %i creation failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_SYNC_OBJECTS_FAILURE, 2)
        }
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorPool(Context *this) {
    VkResult result;

    VkDescriptorPoolSize descriptorPoolSizes[2];
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {0};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(descriptorPoolSizes[0]);
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    result = vkCreateDescriptorPool(this->vk.device, &descriptorPoolCreateInfo, NULL, &this->vk.descriptorPool);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateDescriptorPool failed with result: %i", result);
        RETURN_RESULT_CODE(VE_ALLOC_DESCRIPTOR_POOL_FAILURE, 0)
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static VEngineResult allocateDescriptorSets(Context *this) {
    VkResult result;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = NULL;
    descriptorSetAllocateInfo.descriptorPool = this->vk.descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &this->vk.descriptorSetLayout;

    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(VBufferUniformBufferObject);

    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = this->vk.defaultTextureSampler;
    descriptorImageInfo.imageView = this->vk.texture.imageView;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeDescriptorSets[2] = {{0}, {0}};
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

    for(uint32_t i = MAX_FRAMES_IN_FLIGHT; i != 0; i--) {
        result = vkAllocateDescriptorSets(this->vk.device, &descriptorSetAllocateInfo, &this->vk.frames[i - 1].descriptorSet);

        if(result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkAllocateDescriptorSets at index %i failed with result: %i", i - 1, result);
            RETURN_RESULT_CODE(VE_ALLOC_DESCRIPTOR_SET_FAILURE, i - 1)
        }

        descriptorBufferInfo.buffer = this->vk.frames[i - 1].uniformBuffer;

        writeDescriptorSets[0].dstSet = this->vk.frames[i - 1].descriptorSet;
        writeDescriptorSets[1].dstSet = this->vk.frames[i - 1].descriptorSet;

        vkUpdateDescriptorSets(this->vk.device, 2, writeDescriptorSets, 0, NULL);
    }
    RETURN_RESULT_CODE(VE_SUCCESS, 0)
}

static void cleanupSwapChain(Context *this) {
    vkDestroyImageView( this->vk.device, this->vk.mmaa.imageView,   NULL);
    vkDestroyImage(     this->vk.device, this->vk.mmaa.image,       NULL);
    vkFreeMemory(       this->vk.device, this->vk.mmaa.imageMemory, NULL);
    vkDestroyImageView( this->vk.device, this->vk.depthImageView,   NULL);
    vkDestroyImage(     this->vk.device, this->vk.depthImage,       NULL);
    vkFreeMemory(       this->vk.device, this->vk.depthImageMemory, NULL);

    for(uint32_t i = this->vk.swapChainFrameCount; i != 0; i--) {
        vkDestroyImageView(  this->vk.device, this->vk.pSwapChainFrames[i - 1].imageView,   NULL);
        vkDestroyFramebuffer(this->vk.device, this->vk.pSwapChainFrames[i - 1].framebuffer, NULL);
    }

    if(this->vk.pSwapChainFrames != NULL)
        free(this->vk.pSwapChainFrames);
    this->vk.pSwapChainFrames = NULL;

    vkDestroySwapchainKHR(this->vk.device, this->vk.swapChain, NULL);
}
