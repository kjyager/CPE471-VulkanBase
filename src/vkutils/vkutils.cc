#include "vkutils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <array>

static bool confirm_queue_fam(VkPhysicalDevice aDevice, uint32_t aBitmask);
static int score_physical_device(VkPhysicalDevice aDevice);

namespace vkutils{

std::vector<const char*> strings_to_cstrs(const std::vector<std::string>& aContainer){
   std::vector<const char*> dst(aContainer.size(), nullptr);
   auto lambda_cstr = [](const std::string& str) -> const char* {return(str.c_str());};
   std::transform(aContainer.begin(), aContainer.end(), dst.begin(), lambda_cstr);
   return(dst);
}

VkPhysicalDevice select_physical_device(const std::vector<VkPhysicalDevice>& aDevices){
    int high_score = -1;
    size_t max_index = 0;
    std::vector<int> scores(aDevices.size());
    for(size_t i = 0; i < aDevices.size(); ++i){
        int score = score_physical_device(aDevices[i]);
        if(score > high_score){
            high_score = score;
            max_index = i;
        }
    }
    if(high_score >= 0){
        return(aDevices[max_index]);
    }else{
        return(VK_NULL_HANDLE);
    }
}

VkFormat select_depth_format(const VkPhysicalDevice& aPhysDev, const VkFormat& aPreferred, bool aRequireStencil){
    const static std::array<VkFormat, 5> candidates = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM
    };

    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(aPhysDev, aPreferred, &formatProps);
    bool hasFeature = formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    bool hasStencil = aPreferred >= VK_FORMAT_D16_UNORM_S8_UINT;
    if(hasFeature && (!aRequireStencil || hasStencil)) return(aPreferred);

    for(const VkFormat& format: candidates){
        vkGetPhysicalDeviceFormatProperties(aPhysDev, format, &formatProps);
        bool hasFeature = formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        bool hasStencil = format >= VK_FORMAT_D16_UNORM_S8_UINT;
        if(hasFeature && (!aRequireStencil || hasStencil)) return(format);
    }

    throw std::runtime_error("Failed to find compatible depth format!");
}

VkShaderModule load_shader_module(const VkDevice& aDevice, const std::string& aFilePath){
    std::ifstream shaderFile(aFilePath, std::ios::in | std::ios::binary | std::ios::ate);
    if(!shaderFile.is_open()){
        perror(aFilePath.c_str());
        throw std::runtime_error("Failed to open shader file" + aFilePath + "!");
    }
    size_t fileSize = static_cast<size_t>(shaderFile.tellg());
    std::vector<uint8_t> byteCode(fileSize);
    shaderFile.seekg(std::ios::beg);
    shaderFile.read(reinterpret_cast<char*>(byteCode.data()), fileSize);
    shaderFile.close();

    VkShaderModule resultModule = create_shader_module(aDevice, byteCode, true);
    if(resultModule == VK_NULL_HANDLE){
        std::cerr << "Failed to create shader module from '" << aFilePath << "'!" << std::endl;
    }
    return(resultModule);
}

VkShaderModule create_shader_module(const VkDevice& aDevice, const std::vector<uint8_t>& aByteCode, bool silent){
    VkShaderModuleCreateInfo createInfo;{
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.codeSize = aByteCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(aByteCode.data());
    }

    VkShaderModule resultModule = VK_NULL_HANDLE;
    if(vkCreateShaderModule(aDevice, &createInfo, nullptr, &resultModule) != VK_SUCCESS && !silent){
        std::cerr << "Failed to build shader from byte code!" << std::endl;
    }
    return(resultModule);
}

VkCommandBuffer QueueClosure::beginOneSubmitCommands(VkCommandPool aCommandPool){
    // Create a one off command pool internally
    if(aCommandPool == VK_NULL_HANDLE){
        _mCmdPoolInternal = true;
        VkCommandPoolCreateInfo poolCreate = {};
        poolCreate.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreate.queueFamilyIndex = mFamilyIdx;
        poolCreate.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VkResult poolCreateResult = vkCreateCommandPool(_mDevicePair.device, &poolCreate, nullptr, &_mCommandPool);
        assert(poolCreateResult == VK_SUCCESS);
        aCommandPool = _mCommandPool;
    }
    
    VkCommandBufferAllocateInfo allocInfo = {};
    {
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = aCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    VkCommandBufferBeginInfo beginInfo = {};
    {
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    assert(vkAllocateCommandBuffers(_mDevicePair.device, &allocInfo, &cmdBuffer) == VK_SUCCESS);
    assert(vkBeginCommandBuffer(cmdBuffer, &beginInfo) == VK_SUCCESS);

    return(cmdBuffer);
}

VkResult QueueClosure::finishOneSubmitCommands(const VkCommandBuffer& aCmdBuffer){
    VkSubmitInfo submission = sSingleSubmitTemplate;
    submission.commandBufferCount = 1;
    submission.pCommandBuffers = &aCmdBuffer;
    
    VkResult submitResult = vkQueueSubmit(mQueue, 1, &submission, VK_NULL_HANDLE);
    if(submitResult == VK_SUCCESS) vkQueueWaitIdle(mQueue);
    _cleanupSubmit(aCmdBuffer);
    return(submitResult);
}

void QueueClosure::_cleanupSubmit(const VkCommandBuffer& aCmdBuffer){
    if(aCmdBuffer != VK_NULL_HANDLE && _mCmdPoolInternal){
        vkFreeCommandBuffers(_mDevicePair.device, _mCommandPool, 1, &aCmdBuffer);
    }
    if(_mCommandPool != VK_NULL_HANDLE){
        vkDestroyCommandPool(_mDevicePair.device, _mCommandPool, nullptr);
        _mCommandPool = VK_NULL_HANDLE;
    }
    _mCmdPoolInternal = false;
}

} // end namespace vkutils


static bool confirm_queue_fam(VkPhysicalDevice aDevice, uint32_t aBitmask){
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueCount, queueProperties.data());

    uint32_t maskResult = 0;
    for(const VkQueueFamilyProperties& queueFamily : queueProperties){
        if(queueFamily.queueCount > 0)
            maskResult = maskResult | (aBitmask & queueFamily.queueFlags);
    }

    return(maskResult == aBitmask);
}

static int score_physical_device(VkPhysicalDevice aDevice){
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(aDevice, &properties);
    vkGetPhysicalDeviceFeatures(aDevice, &features);

    int score = 0;

    switch(properties.deviceType){
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            score += 0000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 2000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 4000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 3000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 1000;
            break;
        default:
            break;
    }

    score = confirm_queue_fam(aDevice, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) ? score : -1;

    //TODO: More metrics

    return(score);
}