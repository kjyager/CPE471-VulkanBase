#ifndef APPLICATION_H_
#define APPLICATION_H_
#include "BasicVulkanApp.h"
#include <unordered_map>

class Application : BasicVulkanApp{
 public:
    
    void init();
    void run();
    void cleanup();

 protected:

    void render();

    void initGraphicsPipeline();
    void initRenderPasses();
    void initFramebuffers();
    void initCommands();
    void initSync();

    VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mGraphicsPipeLayout = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkViewport mViewport;
    size_t mFrameNumber = 0;

    const static int IN_FLIGHT_FRAME_LIMIT = 2;

    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishSemaphores;
    std::vector<VkFence> mInFlightFences;

    std::vector<VkFramebuffer> mSwapchainFramebuffers;

    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    std::unordered_map<std::string, VkShaderModule> mShaderModules;
};

#endif