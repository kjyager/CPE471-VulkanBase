#ifndef VULKAN_SWAPCHAIN_APP_H_
#define VULKAN_SWAPCHAIN_APP_H_

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "vkutils/vkutils.h"
#include "VulkanAppInterface.h"
#include "vkutils/VulkanDevices.h"

class SwapchainProvider : virtual public PresentationProviderInterface {
 public:

    virtual ~SwapchainProvider() = default;

    void initGlfw();

    virtual void initPresentationSurface() override;
    virtual void initSwapchain() override;
    virtual void initSwapchainViews() override;

    virtual void cleanup() override;
    virtual void cleanupSwapchain() override;

    virtual void setPresentationExtent(const VkExtent2D& aExtent) {mViewportExtent = aExtent;}

    virtual const vkutils::VulkanSwapchainBundle& getSwapchainBundle() const override {return(mSwapchainBundle);}
    virtual GLFWwindow* getWindowPtr() const override {return(mWindow);}
    virtual VkSurfaceKHR getPresentationSurface() const override{return(mVkSurface);}
    virtual const VkExtent2D& getPresentationExtent() const override {return(mViewportExtent);}

    struct WindowFlags
    {
        bool resized = false;
        bool iconified = false;
        bool focus = true;
    };

    static std::unordered_map<GLFWwindow*, WindowFlags> sWindowFlags;

 protected:
    
    virtual VkQueueFlags getRequiredQueueFlags() const override {return(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);}

    virtual const std::vector<std::string>& getRequiredInstanceExtensions() const override;
    virtual const std::vector<std::string>& getRequiredDeviceExtensions() const override;
    // Functions used to determine swap chain configuration
    virtual const VkSurfaceFormatKHR selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& aFormats) const;
    virtual const VkPresentModeKHR selectPresentationMode(const std::vector<VkPresentModeKHR>& aModes) const;
    virtual const VkExtent2D selectSwapChainExtent(const VkSurfaceCapabilitiesKHR& aCapabilities) const;

    GLFWwindow* mWindow = nullptr;

    VkExtent2D mViewportExtent = {854, 480};

    VkSurfaceKHR mVkSurface = VK_NULL_HANDLE;

    vkutils::VulkanSwapchainBundle mSwapchainBundle;
};

#endif
