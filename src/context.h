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
        VkPhysicalDevice physicalDevice;
        VkQueueFamilyProperties *pQueueFamilyProperties;
        VkSurfaceKHR surface;
        Uint32 queueFamilyPropertyCount;

        Uint32  graphicsQueueFamilyIndex;
        VkQueue graphicsQueue;
        Uint32  presentationQueueFamilyIndex;
        VkQueue presentationQueue;

        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D swapExtent;

        VkSwapchainKHR swapChain;
        Uint32 swapChainImageCount;
        VkImage *pSwapChainImages;
        VkImageView *pSwapChainImageViews;
        VkFramebuffer *pSwapChainFramebuffers;

        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkSurfaceFormatKHR *pSurfaceFormat;
        Uint32 surfaceFormatCount;
        VkPresentModeKHR *pPresentMode;
        Uint32 presentModeCount;
    } vk;

};

extern struct Context context;

#endif // CONTEXT_29
