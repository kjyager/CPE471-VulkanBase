#ifndef APPLICATION_H_
#define APPLICATION_H_
#include "VulkanSetupBaseApp.h"
#include "vkutils/vkutils.h"
#include "data/VertexGeometry.h"
#include <unordered_map>

class VulkanGraphicsApp : public VulkanSetupBaseApp{
 public:
    
    void init();
    void cleanup();

 protected:

    void render();

    void setVertexInput(
       const VkVertexInputBindingDescription& aBindingDescription,
       const std::vector<VkVertexInputAttributeDescription>& aAttributeDescriptions
    );

    void setVertexBuffer(const VkBuffer& aBuffer, size_t aVertexCount);

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

 private:
    bool mVertexInputsHaveBeenSet = false;
    VkVertexInputBindingDescription mBindingDescription = {};
    std::vector<VkVertexInputAttributeDescription> mAttributeDescriptions;
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    size_t mVertexCount = 0U;
};

#endif
