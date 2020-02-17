#ifndef VULKAN_SETUP_BASE_APP_H_
#define VULKAN_SETUP_BASE_APP_H_

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <set>
#include "VulkanAppInterface.h"
#include "vkutils/vkutils.h"

class VulkanSetupCore : virtual public CoreProviderInterface
{
 public:
    virtual ~VulkanSetupCore() = default;

    virtual void registerDependentProvider(const VulkanProviderInterface* aProvider) override;
    virtual void linkPresentationProvider(const PresentationProviderInterface* aProvider) override;

    virtual void linkHostApp(const VulkanAppInterface* aHost) override {mHostApp = aHost;}

    /** Shortcut init which calls the other init functions in order.
     *   1. initVkInstance();
     *   2. initVkPhysicalDevice();
     *   3. initVkLogicalDevice();
     *   4. Registers device bundle with VmaHost singleton
     */
    void initVulkan();

    /// Initializes a vulkan instance with default application info,
    void initVkInstance();
    virtual void initVkInstance(const VkApplicationInfo& aAppInfo) override;
    virtual void initVkPhysicalDevice() override;
    virtual void initVkLogicalDevice() override;

    virtual void cleanup() override;

    virtual const VkInstance getVulkanInstance() const override {return(mVkInstance);}
    virtual const VulkanDeviceBundle& getPrimaryDeviceBundle() const override {return(mDeviceBundle);}

    virtual VkQueueFlags getRequiredQueueFlags() const override;
    
    std::vector<std::string> gatherInstanceExtensions();
    std::vector<std::string> gatherDeviceExtensions();
    std::vector<std::string> gatherValidationLayers();

    virtual const std::unordered_map<std::string, bool>& getValidationLayersState() const override {return(_mValidationLayers);}
    virtual const std::unordered_map<std::string, bool>& getInstanceExtensionState() const override {return(_mInstExtensions);}

    virtual std::unordered_map<std::string, bool>& getMutValidationLayersState() override {return(_mValidationLayers);}
    virtual std::unordered_map<std::string, bool>& getMutInstanceExtensionState() override {return(_mInstExtensions);}

    VkInstance mVkInstance = VK_NULL_HANDLE;
    VulkanDeviceBundle mDeviceBundle;
    
    const PresentationProviderInterface* mPresentationProvider = nullptr;
    const VulkanAppInterface* mHostApp = nullptr;

 private:
    std::set<const VulkanProviderInterface*> _mDependentProviders; 

    std::unordered_map<std::string, bool> _mValidationLayers;
    std::unordered_map<std::string, bool> _mInstExtensions; 
};

#endif
