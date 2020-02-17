#include "vkutils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <iterator>

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
    if(vkCreateShaderModule(aDevice, &createInfo, nullptr, &resultModule) != VK_SUCCESS){
        std::cerr << "Failed to build shader from byte code!" << std::endl;
    }
    return(resultModule);
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