#include "vkutils.h"
#include <iostream>
#include <algorithm>
#include <cassert>

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
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->extensionName, true);
        }else{
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->extensionName, false);
            std::cerr << "Warning: Requested extension " + std::string(ext_name) + " is not available" << std::endl;
        }
    }

    for(std::string ext_name : aRequired){
        auto streq = [ext_name](const VkExtensionProperties& other) -> bool {return(ext_name == other.extensionName);};
        std::vector<VkExtensionProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->extensionName);
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->extensionName, true);
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
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->layerName, true);
        }else{
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->layerName, false);
            std::cerr << "Warning: Requested validation layer " + std::string(layer_name) + " is not available" << std::endl;
        }
    }

    for(std::string layer_name : aRequired){
        auto streq = [layer_name](const VkLayerProperties& other) -> bool {return(layer_name == other.layerName);};
        std::vector<VkLayerProperties>::const_iterator match = std::find_if(aAvailable.begin(), aAvailable.end(), streq);
        if(match != aAvailable.end()){
            aOutExtList.emplace_back(match->layerName);
            if(aResultMap != nullptr) aResultMap->insert_or_assign(match->layerName, true);
        }else{
            throw std::runtime_error("Required instance extension " + std::string(layer_name) + " is not available!");
        }
    }
}

} // end namespace vkutils
