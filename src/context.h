#ifndef CONTEXT_29
#define CONTEXT_29

#include "SDL.h"
#include <vulkan/vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2

struct Context {
    char title[64];
    int x, y;
    int w, h;
    Uint32 flags;
    SDL_Window *pWindow;
    int forceSwapChainRegen;

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
        Uint32 swapChainFrameCount;
        struct {
            VkImage       image;
            VkImageView   imageView;
            VkFramebuffer framebuffer;
        } *pSwapChainFrames;

        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        VkCommandPool commandPool;

        struct {
            VkCommandBuffer commandBuffer;
            VkSemaphore imageAvailableSemaphore;
            VkSemaphore renderFinishedSemaphore;
            VkFence inFlightFence;
        } frames[MAX_FRAMES_IN_FLIGHT];
        unsigned currentFrame;
    } vk;

};

extern struct Context context;

#endif // CONTEXT_29
