#ifndef APPLICATION_H_
#define APPLICATION_H_
#include "BasicVulkanApp.h"
#include "vkutils.h"
#include <unordered_map>

class Application : BasicVulkanApp{
 public:
    
    void init();
    void run();
    void cleanup();

 protected:

    void render();

    void initRenderPipeline();
    void initFramebuffers();
    void initCommands();
    void initSync();

    void resetRenderSetup();

    void cleanupSwapchainDependents();

    size_t mFrameNumber = 0;

    const static int IN_FLIGHT_FRAME_LIMIT = 2;
    std::vector<VkFramebuffer> mSwapchainFramebuffers;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishSemaphores;
    std::vector<VkFence> mInFlightFences;

    vkutils::BasicVulkanRenderPipeline mRenderPipeline;

    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    std::unordered_map<std::string, VkShaderModule> mShaderModules;
};

#endif