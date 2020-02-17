#pragma once
#ifndef LOAD_VULKAN_H_
#define LOAD_VULKAN_H_
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <functional>
#include "VulkanDevices.h"

namespace vkutils{

std::vector<const char*> strings_to_cstrs(const std::vector<std::string>& aContainer);

template<typename ContainerType>
void find_extension_matches(
    const std::vector<VkExtensionProperties>& aAvailable,
    const ContainerType& aRequired, const ContainerType& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap = nullptr
);

template<typename ContainerType>
void find_layer_matches(
    const std::vector<VkLayerProperties>& aAvailable,
    const ContainerType& aRequired, const ContainerType& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap = nullptr
);

VkPhysicalDevice select_physical_device(const std::vector<VkPhysicalDevice>& aDevices);

template<typename T>
std::vector<T>& duplicate_extend_vector(std::vector<T>& aVector, size_t extendSize);

VkShaderModule load_shader_module(const VkDevice& aDevice, const std::string& aFilePath);
VkShaderModule create_shader_module(const VkDevice& aDevice, const std::vector<uint8_t>& aByteCode, bool silent = false);

const static VkSubmitInfo sSingleSubmitTemplate {
    /* sType = */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
    /* pNext = */ nullptr,
    /* waitSemaphoreCount = */ 0,
    /* pWaitSemaphores = */ nullptr,
    /* pWaitDstStageMask = */ 0,
    /* commandBufferCount = */ 1,
    /* pCommandBuffers = */ nullptr,
    /* signalSemaphoreCount = */ 0,
    /* pSignalSemaphores = */ nullptr
};

// Inline include render pipeline components
#include "vkutils_VulkanRenderPipeline.inl"

// Inline include compute pipeline components
#include "vkutils_VulkanComputePipeline.inl"

} // end namespace vkutils

template<typename ContainerType>
void vkutils::find_extension_matches(
    const std::vector<VkExtensionProperties>& aAvailable,
    const ContainerType& aRequired, const ContainerType& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap
){
    for(std::string ext_name : aRequested){
        auto streq = [ext_name](const VkExtensionProperties& other) -> bool {return(ext_name == other.extensionName);};
        std::vector<VkExtensionProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->extensionName);
            if(aResultMap != nullptr){
                aResultMap->operator[](match->extensionName) = true;
            }
        }else{
            if(aResultMap != nullptr){
                aResultMap->operator[](ext_name) = false;
            }
            std::cerr << "Warning: Requested extension " + std::string(ext_name) + " is not available" << std::endl;
        }
    }

    for(std::string ext_name : aRequired){
        auto streq = [ext_name](const VkExtensionProperties& other) -> bool {return(ext_name == other.extensionName);};
        std::vector<VkExtensionProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->extensionName);
            if(aResultMap != nullptr){
                aResultMap->operator[](match->extensionName) = true;
            }
        }else{
            throw std::runtime_error("Required instance extension " + std::string(ext_name) + " is not available!");
        }
    }
}

template<typename ContainerType>
void vkutils::find_layer_matches(
    const std::vector<VkLayerProperties>& aAvailable,
    const ContainerType& aRequired, const ContainerType& aRequested,
    std::vector<std::string>& aOutExtList, std::unordered_map<std::string, bool>* aResultMap
){
    for(std::string layer_name : aRequested){
        auto streq = [layer_name](const VkLayerProperties& other) -> bool {return(layer_name == other.layerName);};
        std::vector<VkLayerProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->layerName);
            if(aResultMap != nullptr){
                aResultMap->operator[](match->layerName) = true;
            }
        }else{
            if(aResultMap != nullptr){
                aResultMap->operator[](layer_name) = false;
            }
            std::cerr << "Warning: Requested validation layer " + std::string(layer_name) + " is not available" << std::endl;
        }
    }

    for(std::string layer_name : aRequired){
        auto streq = [layer_name](const VkLayerProperties& other) -> bool {return(layer_name == other.layerName);};
        std::vector<VkLayerProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->layerName);
            if(aResultMap != nullptr){
                aResultMap->operator[](match->layerName) = true;
            }
        }else{
            throw std::runtime_error("Required instance extension " + std::string(layer_name) + " is not available!");
        }
    }
}

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