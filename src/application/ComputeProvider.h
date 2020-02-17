#ifndef VULKAN_COMPUTE_APP_H_
#define VULKAN_COMPUTE_APP_H_
#include "VulkanAppInterface.h"
#include "vkutils/vkutils.h"
#include "vkutils/VulkanDevices.h"
#include <unordered_map>
#include <vector>

class ComputeProvider : virtual public ComputeProviderInterface
{
 public:
    void init();
    void cleanup();

 protected:

    VkCommandPool mComputeCommandPool = VK_NULL_HANDLE;
    
    std::unordered_map<std::string, vkutils::VulkanComputePipeline> mComputePipelines;
    std::unordered_map<std::string, VkShaderModule> mShaderModules; 
};

#endif