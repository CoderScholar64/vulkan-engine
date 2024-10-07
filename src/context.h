#ifndef CONTEXT_29
#define CONTEXT_29

#include "SDL.h"
#include <vulkan/vulkan.h>

#include "v_buffer_def.h"
#include "v_model_def.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Context {
    char title[64];
    int x, y;
    int w, h;
    uint32_t flags;
    SDL_Window *pWindow;
    int forceSwapChainRegen;

    Vector3 position;
    float yaw;
    float pitch;
    Matrix modelView;

    struct {
        VkDevice device;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkQueueFamilyProperties *pQueueFamilyProperties;
        VkSurfaceKHR surface;
        uint32_t queueFamilyPropertyCount;

        uint32_t  graphicsQueueFamilyIndex;
        VkQueue graphicsQueue;
        uint32_t  presentationQueueFamilyIndex;
        VkQueue presentationQueue;

        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D swapExtent;

        VkSwapchainKHR swapChain;
        uint32_t swapChainFrameCount;
        struct {
            VkImage       image;
            VkImageView   imageView;
            VkFramebuffer framebuffer;
        } *pSwapChainFrames;

        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        unsigned modelAmount;
        VModelData *pModels;
        unsigned modelArrayAmount;
        VModelArray **ppVModelArray;

        VkCommandPool commandPool;

        struct {
            uint32_t mipLevels;
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory imageMemory;
        } texture;

        struct {
            VkSampleCountFlagBits samples;
            VkImage image;
            VkDeviceMemory imageMemory;
            VkImageView imageView;
        } mmaa;

        VkFormat depthFormat;
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkSampler defaultTextureSampler;

        struct {
            VkCommandBuffer commandBuffer;
            VkDescriptorSet descriptorSet;
            VkSemaphore imageAvailableSemaphore;
            VkSemaphore renderFinishedSemaphore;
            VkFence inFlightFence;
            VkBuffer uniformBuffer;
            VkDeviceMemory uniformBufferMemory;
            void* uniformBufferMapped;
        } frames[MAX_FRAMES_IN_FLIGHT];
        VkDescriptorPool descriptorPool;
        unsigned currentFrame;
    } vk;

} Context;

#endif // CONTEXT_29
