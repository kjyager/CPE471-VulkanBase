#ifndef VULKAN_SETUP_BASE_APP_H_
#define VULKAN_SETUP_BASE_APP_H_

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "vkutils/vkutils.h"

class VulkanSetupBaseApp{
 public:

    void init();
    void cleanup();

    struct WindowFlags
    {
        bool resized = false;
        bool iconified = false;
        bool focus = true;
    };

    static std::unordered_map<GLFWwindow*, WindowFlags> sWindowFlags;

 protected:

    void initGlfw();
    void initVulkan();

    void createVkInstance();
    void initPresentationSurface();
    void initVkDevices();
    void initSwapchain();
    void initSwapchainViews();

    void cleanupSwapchain();

    std::vector<std::string> gatherExtensionInfo();
    std::vector<std::string> gatherValidationLayers();

    // Virtual functions for retreving information about the setup of the applications vulkan 
    // instance and devices. Overriding these functions in derived classes allow modifying the 
    // Vulkan setup with minimal amounts of new or re-written logic. 
    virtual const VkApplicationInfo& getAppInfo() const;
    virtual const std::vector<std::string>& getRequiredValidationLayers() const;
    virtual const std::vector<std::string>& getRequestedValidationLayers() const;
    virtual const std::vector<std::string>& getRequiredInstanceExtensions() const;
    virtual const std::vector<std::string>& getRequestedInstanceExtensions() const;
    virtual const std::vector<std::string>& getRequiredDeviceExtensions() const;
    virtual const std::vector<std::string>& getRequestedDeviceExtensions() const;
    // Functions used to determine swap chain configuration
    virtual const VkSurfaceFormatKHR selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& aFormats) const;
    virtual const VkPresentModeKHR selectPresentationMode(const std::vector<VkPresentModeKHR>& aModes) const;
    virtual const VkExtent2D selectSwapChainExtent(const VkSurfaceCapabilitiesKHR& aCapabilities) const;

    const std::unordered_map<std::string, bool>& getValidationLayersState() const;
    const std::unordered_map<std::string, bool>& getExtensionState() const; 

    GLFWwindow* mWindow = nullptr;

    VkExtent2D mViewportExtent = {854, 480};

    VkInstance mVkInstance;
    VulkanDeviceBundle mDeviceBundle;
    VkSurfaceKHR mVkSurface;
    VkQueue mGraphicsQueue;
    VkQueue mPresentationQueue;

    vkutils::VulkanSwapchainBundle mSwapchainBundle;

 private:

    std::unordered_map<std::string, bool> _mValidationLayers;
    std::unordered_map<std::string, bool> _mExtensions; 
    

};

#endif
