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

} // end namespace vkutils



#endif