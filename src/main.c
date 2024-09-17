#include "SDL.h"
#include <vulkan/vulkan.h>
#include "SDL_vulkan.h"

struct Context {
    char title[64];
    int x, y;
    int w, h;
    Uint32 flags;
    SDL_Window *pWindow;

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkQueueFamilyProperties *pQueueFamilyProperties;
    Uint32 queueFamilyPropertyCount;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surface;

} context = {"Hello World", 0, 0, 1920, 1080, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN};

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

    VkInstanceCreateInfo instanceCreateInfo;
    memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = ppExtensionNames;

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &context.instance);

    if(ppExtensionNames != NULL)
        free(ppExtensionNames);

    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateInstance returned %i", result);
        return -4;
    }

    return 1;
}

VkQueueFamilyProperties* allocateQueueFamilyArray(VkPhysicalDevice device, Uint32 *queueFamilyPropertyCount) {
    *queueFamilyPropertyCount = 0;
    VkQueueFamilyProperties *pQueueFamilyProperties = NULL;

    vkGetPhysicalDeviceQueueFamilyProperties(device, queueFamilyPropertyCount, NULL);

    if(*queueFamilyPropertyCount != 0)
        pQueueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * (*queueFamilyPropertyCount));

    if(pQueueFamilyProperties != NULL)
        vkGetPhysicalDeviceQueueFamilyProperties(device, queueFamilyPropertyCount, pQueueFamilyProperties);

    return pQueueFamilyProperties;
}

int findPhysicalDevice() {
    context.physicalDevice = NULL;
    context.pQueueFamilyProperties = NULL;
    context.queueFamilyPropertyCount = 0;

    Uint32 physicalDevicesCount = 0;
    VkPhysicalDevice *pPhysicalDevices = NULL;

    VkResult result = vkEnumeratePhysicalDevices(context.instance, &physicalDevicesCount, NULL);
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

    result = vkEnumeratePhysicalDevices(context.instance, &physicalDevicesCount, pPhysicalDevices);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkEnumeratePhysicalDevices for devices returned %i", result);
        free(pPhysicalDevices);
        return -8;
    }
    VkPhysicalDeviceProperties physicalDeviceProperties;

    unsigned int deviceIndex = physicalDevicesCount;
    Uint32 queueFamilyPropertyCount;
    VkQueueFamilyProperties *pQueueFamilyProperties;
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
        }

        free(pQueueFamilyProperties);

        if(requiredParameters == 1) {
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
        context.physicalDevice = pPhysicalDevices[deviceIndex];

        context.pQueueFamilyProperties = allocateQueueFamilyArray(context.physicalDevice, &context.queueFamilyPropertyCount);
    }

    free(pPhysicalDevices);

    return 1;
}

int allocateLogicalDevice() {
    context.device = NULL;

    float normal_priority = 1.0f;
    VkResult result;
    VkBool32 surfaceSupported;
    const unsigned GRAPHICS_FAMILY_INDEX = 0;
    const unsigned PRESENT_FAMILY_INDEX  = 1;

    VkDeviceQueueCreateInfo deviceQueueCreateInfos[2];
    memset(deviceQueueCreateInfos, 0, sizeof(VkDeviceQueueCreateInfo));

    for(int i = 0; i < 2; i++) {
        deviceQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfos[i].pNext = NULL;
        deviceQueueCreateInfos[i].pQueuePriorities = &normal_priority;
        deviceQueueCreateInfos[i].queueCount = 1;
    }

    for(Uint32 p = context.queueFamilyPropertyCount; p == 0; p--) {
        if( (context.pQueueFamilyProperties[p - 1].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) {
            deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex = p - 1;
        }

        result = vkGetPhysicalDeviceSurfaceSupportKHR(context.physicalDevice, p - 1, context.surface, &surfaceSupported);

        if(result != VK_SUCCESS) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "At index %i for vkGetPhysicalDeviceSurfaceSupportKHR returned %i", p - 1, result);
        }

        if(surfaceSupported == VK_TRUE)
            deviceQueueCreateInfos[PRESENT_FAMILY_INDEX].queueFamilyIndex = p - 1;
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    memset(&physicalDeviceFeatures, 0, sizeof(physicalDeviceFeatures));

    VkDeviceCreateInfo deviceCreateInfo;
    memset(&deviceCreateInfo, 0, sizeof(deviceCreateInfo));
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
    deviceCreateInfo.queueCreateInfoCount = 2;

    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.enabledLayerCount = 0;

    result = vkCreateDevice(context.physicalDevice, &deviceCreateInfo, NULL, &context.device);
    if(result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %i", result);
        return -9;
    }

    vkGetDeviceQueue(context.device, deviceQueueCreateInfos[GRAPHICS_FAMILY_INDEX].queueFamilyIndex, 0, &context.graphicsQueue);
    vkGetDeviceQueue(context.device, deviceQueueCreateInfos[ PRESENT_FAMILY_INDEX].queueFamilyIndex, 0, &context.presentationQueue);

    return 1;

}

int initVulkan() {
    context.instance = NULL;

    int returnCode;

    returnCode = initInstance();
    if( returnCode < 0 )
        return returnCode;

    returnCode = findPhysicalDevice();
    if( returnCode < 0 )
        return returnCode;

    if(SDL_Vulkan_CreateSurface(context.pWindow, context.instance, &context.surface) != SDL_TRUE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rendering device returned %s", SDL_GetError());
        returnCode = -10;
    }

    returnCode = allocateLogicalDevice();
    if( returnCode < 0 )
        return returnCode;

    return 1;
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

    if(context.queueFamilyPropertyCount > 0)
        free(context.pQueueFamilyProperties);

    vkDestroyDevice(context.device, NULL);
    vkDestroySurfaceKHR(context.instance, context.surface, NULL);
    vkDestroyInstance(context.instance, NULL);
    SDL_DestroyWindow(context.pWindow);

    return returnCode;
}
