#include "vkutils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <iterator>

namespace vkutils{

std::vector<const char*> strings_to_cstrs(const std::vector<std::string>& aContainer){
   std::vector<const char*> dst(aContainer.size(), nullptr);
   auto lambda_cstr = [](const std::string& str) -> const char* {return(str.c_str());};
   std::transform(aContainer.begin(), aContainer.end(), dst.begin(), lambda_cstr);
   return(dst);
}

void find_extension_matches(
    const std::vector<VkExtensionProperties>& aAvailable,
    const std::vector<std::string>& aRequired, const std::vector<std::string>& aRequested,
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

void find_layer_matches(
    const std::vector<VkLayerProperties>& aAvailable,
    const std::vector<std::string>& aRequired, const std::vector<std::string>& aRequested,
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
