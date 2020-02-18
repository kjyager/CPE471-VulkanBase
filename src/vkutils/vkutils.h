#pragma once
#ifndef LOAD_VULKAN_H_
#define LOAD_VULKAN_H_
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "VulkanDevices.h"

namespace vkutils{

std::vector<const char*> strings_to_cstrs(const std::vector<std::string>& aContainer);

void find_extension_matches(
    const std::vector<VkExtensionProperties>& aAvailable,
    const std::vector<std::string>& aRequired, const std::vector<std::string>& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap = nullptr
);
void find_layer_matches(
    const std::vector<VkLayerProperties>& aAvailable,
    const std::vector<std::string>& aRequired, const std::vector<std::string>& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap = nullptr
);

template<typename T>
std::vector<T>& duplicate_extend_vector(std::vector<T>& aVector, size_t extendSize);

template<typename T>
std::vector<T> duplicate_extend_vector(const std::vector<T>& aVector, size_t extendSize);

VkShaderModule load_shader_module(const VkDevice& aDevice, const std::string& aFilePath);
VkShaderModule create_shader_module(const VkDevice& aDevice, const std::vector<uint8_t>& aByteCode, bool silent = false);

struct VulkanSwapchainBundle
{
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR presentation_mode;
    VkExtent2D extent = {0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t requested_image_count = 0;
    uint32_t image_count = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

class RenderPassConstructionSet
{
 public:

    // Device handle needed for VkCreate functions
    VkDevice mLogicalDevice = VK_NULL_HANDLE;

    // Swapchain information is needed for setting up parts of 
    // the render pass. 
    VulkanSwapchainBundle const* mSwapchainBundle = nullptr;

    VkAttachmentDescription mColorAttachment;
    VkAttachmentReference mAttachmentRef;
    VkSubpassDescription mSubpass;
    VkSubpassDependency mDependency;

 protected:
    friend class GraphicsPipelineConstructionSet;
    friend class BasicVulkanRenderPipeline;
    RenderPassConstructionSet(){}
    RenderPassConstructionSet(const VkDevice& aDevice, const VulkanSwapchainBundle* aChainBundle)
    :   mLogicalDevice(aDevice), mSwapchainBundle(aChainBundle) {}
};

class GraphicsPipelineConstructionSet
{
 public:

    // Device handle needed for VkCreate functions
    VkDevice mLogicalDevice = VK_NULL_HANDLE;

    // Swapchain information is needed for setting up parts of 
    // the pipeline. 
    VulkanSwapchainBundle const* mSwapchainBundle = nullptr;

    // Sub construction set for the render pass
    RenderPassConstructionSet mRenderpassCtorSet;

    // It's assumed all programmable stages will be added by the user.
    std::vector<VkPipelineShaderStageCreateInfo> mProgrammableStages;

    // It's assumed most of these will be set up using boiler plate code
    // Intervention from the user is not expected, but is certainly allowed
    VkPipelineVertexInputStateCreateInfo mVtxInputInfo;
    VkPipelineInputAssemblyStateCreateInfo mInputAsmInfo;
    VkViewport mViewport;
    VkRect2D mScissor;
    VkPipelineRasterizationStateCreateInfo mRasterInfo;
    VkPipelineMultisampleStateCreateInfo mMultisampleInfo;
    VkPipelineColorBlendAttachmentState mBlendAttachmentInfo;
    VkPipelineColorBlendStateCreateInfo mColorBlendInfo;
    VkPipelineLayoutCreateInfo mPipelineLayoutInfo;
    std::vector<VkDynamicState> mDynamicStates;

 protected:
    friend class BasicVulkanRenderPipeline;
    GraphicsPipelineConstructionSet(){}
    GraphicsPipelineConstructionSet(const VkDevice& aDevice, const VulkanSwapchainBundle* aChainBundle)
    :   mLogicalDevice(aDevice), mSwapchainBundle(aChainBundle), mRenderpassCtorSet(aDevice, aChainBundle) {}

};

class BasicVulkanRenderPipeline
{
 public:
    BasicVulkanRenderPipeline(){}
   
    /// Setup construction set during object construction.
    BasicVulkanRenderPipeline(const VkDevice& aLogicalDevice, const VulkanSwapchainBundle* aChainBundle);
    ~BasicVulkanRenderPipeline(){
        if(_mValid){
            destroy();
        }
    }

    bool isValid() const {return(_mValid);}

    /// Setup and return a default construction set as a non-const reference
    GraphicsPipelineConstructionSet& setupConstructionSet(const VkDevice& aLogicalDevice, const VulkanSwapchainBundle* aChainBundle);

    /// Fill in the construction set with boilerplate values for a graphics render pipeline
    /// Modifies the given construction set, but does not actual object creation such that 
    /// tweaks can be made to the construction set prior to actual pipeline creation. 
    static void prepareFixedStages(GraphicsPipelineConstructionSet& aCtorSetInOut);
    static void prepareViewport(GraphicsPipelineConstructionSet& aCtorSetInOut);
    static void prepareRenderPass(GraphicsPipelineConstructionSet& aCtorSetInOut);

    /// Submit aFinalCtorSet as the construction set for this pipeline. The pipeline
    /// is then created fresh using the given construction set. The success of this
    /// function will make the object valid and usable. 
    void build(const GraphicsPipelineConstructionSet& aFinalCtorSet);

    /// Recreate the pipeline using the existing construction set. Swapchain information should
    /// still be accessible through the pointer given during construction of this object, so 
    /// an out of sync swapchain should be re-synchronized automatically during recreation. 
    void rebuild();

    // Destroy this pipeline and associated Vulkan objects
    void destroy();

    const VkPipeline& getPipeline() const { return(mGraphicsPipeline); }
    const VkPipelineLayout& getLayout() const { return(mGraphicsPipeLayout); }
    const VkRenderPass& getRenderpass() const { return(mRenderPass); }
    const VkViewport& getViewport() const { return(mViewport); }

 protected:

    VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mGraphicsPipeLayout = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkViewport mViewport;

 private:
    GraphicsPipelineConstructionSet _mConstructionSet;
    VkDevice _mLogicalDevice = VK_NULL_HANDLE;
    bool _mValid = false;
};

} // end namespace vkutils

template<typename T>
std::vector<T>& vkutils::duplicate_extend_vector(std::vector<T>& aVector, size_t extendSize){
    if(aVector.size() == extendSize) return aVector;
    assert(extendSize >= aVector.size() && extendSize % aVector.size() == 0);

    size_t duplicationsNeeded = (extendSize / aVector.size()) - 1;
    aVector.reserve(extendSize);
    auto origBegin = aVector.begin();
    auto origEnd = aVector.end(); 
    for(size_t i = 0; i < duplicationsNeeded; ++i){
        auto dupBegin = aVector.end();
        aVector.insert(dupBegin, origBegin, origEnd);
    }

    assert(aVector.size() == extendSize);
    return(aVector);
}


#endif