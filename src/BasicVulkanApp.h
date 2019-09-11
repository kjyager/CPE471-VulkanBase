#ifndef BASIC_VULKAN_APP_H_
#define BASIC_VULKAN_APP_H_

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "vkutils.h"

class BasicVulkanApp{
 public:

    void init();
    void cleanup();
 protected:

  void initGlfw();
  void initVulkan();
  void createVkInstance();
  void initVkDevices();
  void initPresentationSurface();

  std::vector<std::string> gatherExtensionInfo();
  std::vector<std::string> gatherValidationLayers();

  const VkApplicationInfo& getAppInfo() const;
  const std::vector<std::string>& getRequiredValidationLayers() const;
  const std::vector<std::string>& getRequestedValidationLayers() const;
  const std::vector<std::string>& getRequiredInstanceExtensions() const;
  const std::vector<std::string>& getRequestedInstanceExtensions() const;
  const std::vector<std::string>& getRequiredDeviceExtensions() const;
  const std::vector<std::string>& getRequestedDeviceExtensions() const;
  const std::unordered_map<std::string, bool>& getValidationLayersState() const;
  const std::unordered_map<std::string, bool>& getExtensionState() const; 

  int mViewportWidth = 854;
  int mViewportHeight = 480; 

  VkInstance mVkInstance;
  VulkanPhysicalDevice mPhysDevice;
  VulkanDevice mLogicalDevice;
  VkSurfaceKHR mVkSurface;
  VkQueue mGraphicsQueue;
  VkQueue mPresentationQueue;

  GLFWwindow* mWindow = nullptr;

 private:

   std::unordered_map<std::string, bool> _mValidationLayers;
   std::unordered_map<std::string, bool> _mExtensions; 
    

};

#endif