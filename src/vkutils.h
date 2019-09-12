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

VkShaderModule load_shader_module(const VkDevice& aDevice, const std::string& aFilePath);
VkShaderModule create_shader_module(const VkDevice& aDevice, const std::vector<uint8_t>& aByteCode, bool silent = false);

} // end namespace vkutils



#endif