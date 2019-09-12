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

    void initGraphicsPipeline();
    void initRenderPasses();

    VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mGraphicsPipeLayout = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;

    std::unordered_map<std::string, VkShaderModule> mShaderModules;
};

#endif