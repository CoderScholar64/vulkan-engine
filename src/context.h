#ifndef CONTEXT_29
#define CONTEXT_29

#include "SDL.h"
#include <vulkan/vulkan.h>

struct Context {
    char title[64];
    int x, y;
    int w, h;
    Uint32 flags;
    SDL_Window *pWindow;

    struct {
        VkDevice device;
        VkInstance instance;
        Uint32 graphicsQueueFamilyIndex;
        Uint32 presentationQueueFamilyIndex;
        VkPhysicalDevice physicalDevice;
        VkQueueFamilyProperties *pQueueFamilyProperties;
        VkSurfaceKHR surface;
        Uint32 queueFamilyPropertyCount;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D swapExtent;

        VkSwapchainKHR swapChain;
        Uint32 swapChainImageCount;
        VkImage *pSwapChainImages;
        VkImageView *pSwapChainImageViews;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkSurfaceFormatKHR *pSurfaceFormat;
        Uint32 surfaceFormatCount;
        VkPresentModeKHR *pPresentMode;
        Uint32 presentModeCount;
    } vk;

};

extern struct Context context;

#endif // CONTEXT_29
