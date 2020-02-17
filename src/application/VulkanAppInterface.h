#ifndef KJY_VULKAN_APP_INTERFACE_H_
#define KJY_VULKAN_APP_INTERFACE_H_

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include "vkutils/vkutils.h"
#include "vkutils/VulkanDevices.h"

using VulkanProviderType = uint32_t;
struct VulkanProviderTypeEnum {
    const static VulkanProviderType CORE_PROVIDER_BIT    = 1 << 0;
    const static VulkanProviderType GRAPHICS_BIT         = 1 << 1;
    const static VulkanProviderType COMPUTE_BIT          = 1 << 2;
    const static VulkanProviderType PRESENTATION_BIT     = 1 << 3;
    const static VulkanProviderType SHADER_LIBRARY_BIT   = 1 << 4;
};

class VulkanProviderInterface
{
 public:
   ~VulkanProviderInterface() = default;

    /// Returns bitmask marking the services provided by this class
    virtual VulkanProviderType getProviderTypeBitmask() const = 0;
    /// Returns queue flags required by this provider class. 
    virtual VkQueueFlags getRequiredQueueFlags() const {return(0);}

    virtual const std::vector<std::string>& getRequiredValidationLayers() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedValidationLayers() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequiredInstanceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedInstanceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequiredDeviceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedDeviceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
};

class PresentationProviderInterface;

class VulkanAppInterface;

class CoreProviderInterface : virtual public VulkanProviderInterface
{
 public:
    virtual VulkanProviderType getProviderTypeBitmask() const override {return(VulkanProviderTypeEnum::CORE_PROVIDER_BIT);}

    /** Register 'aProvider' as a dependent provider of this core provider. The dependent
     * provider will be queried about it's requirements during instance and device initialization. 
     * WARNING: Assume there is not protection against creating cycles in such dependencies
     */
    virtual void registerDependentProvider(const VulkanProviderInterface* aProvider) = 0;

    /// Link a presentation provider which may be used during device setup.
    /// Implicitely registers the presentation provider as a dependent as well. 
    virtual void linkPresentationProvider(const PresentationProviderInterface* aProvider) = 0;

    /// Link the host application for purposes of querying required/requested layers and extensions
    virtual void linkHostApp(const VulkanAppInterface* aHost) = 0;

   //  /* Functions allowing host applications to add extra info about initialization */
   //  virtual void setHostRequiredValidationLayers(const std::vector<std::string>& aLayers) = 0;
   //  virtual void setHostRequestedValidationLayers(const std::vector<std::string>& aLayers) = 0;
   //  virtual void setHostRequiredInstanceExtensions(const std::vector<std::string>& aExtensions) = 0;
   //  virtual void setHostRequestedInstanceExtensions(const std::vector<std::string>& aExtensions) = 0;
   //  virtual void setHostRequiredDeviceExtensions(const std::vector<std::string>& aExtensions) = 0;
   //  virtual void setHostRequestedDeviceExtensions(const std::vector<std::string>& aExtensions) = 0;

    /// Initialize a single Vulkan instance using the given application info, and taking dependent providers into consideration.
    virtual void initVkInstance(const VkApplicationInfo& aAppInfo) = 0;
    /// Initialize a single Vulkan physical device taking dependent providers into consideration. 
    virtual void initVkPhysicalDevice() = 0;
    /** Initialize a single Vulkan logical device taking dependent providers into consideration.
     * May pull presentation information from linked presentation provider.
     */
    virtual void initVkLogicalDevice() = 0;

    /// Cleanup instance, physical, and logical devices
    virtual void cleanup() = 0;

    virtual const VkInstance getVulkanInstance() const = 0;
    virtual const VulkanDeviceBundle& getPrimaryDeviceBundle() const = 0;

    /** Get const reference to a mapping from all requested or required validation layers 
     *  to a boolean which is true if the validation layer was enabled successfully.*/
    virtual const std::unordered_map<std::string, bool>& getValidationLayersState() const = 0;
    /** Get const reference to a mapping from all requested or required instance extensions 
     *  to a boolean which is true if the extension was enabled successfully. */
    virtual const std::unordered_map<std::string, bool>& getInstanceExtensionState() const = 0;

 protected:
    /** Non-const version of getValidationLayersState() */
    virtual std::unordered_map<std::string, bool>& getMutValidationLayersState() = 0;
    /** Non-const version of getInstanceExtensionState() */
    virtual std::unordered_map<std::string, bool>& getMutInstanceExtensionState() = 0;

};

class CoreLink
{
 public:
    CoreLink() = default;
    CoreLink(const CoreProviderInterface* aCoreProvider) : mCoreProvider(aCoreProvider) {}

    void setCoreProvider(const CoreProviderInterface* aCoreProvider) {mCoreProvider = aCoreProvider;}
    const VkInstance getVulkanInstance() const {return(mCoreProvider->getVulkanInstance());}
    const VulkanDeviceBundle& getPrimaryDeviceBundle() const {return(mCoreProvider->getPrimaryDeviceBundle());}

 protected:
    const CoreProviderInterface* mCoreProvider = nullptr;
};

class PresentationProviderInterface : virtual public VulkanProviderInterface, public CoreLink
{
 public:
    virtual ~PresentationProviderInterface() = default;
    virtual VulkanProviderType getProviderTypeBitmask() const override {return(VulkanProviderTypeEnum::PRESENTATION_BIT);}

    virtual void initPresentationSurface() = 0;
    virtual void initSwapchain() = 0;
    virtual void initSwapchainViews() = 0;

    virtual GLFWwindow* getWindowPtr() const = 0;
    virtual const vkutils::VulkanSwapchainBundle& getSwapchainBundle() const = 0;
    virtual VkSurfaceKHR getPresentationSurface() const {return(VK_NULL_HANDLE);}
    virtual const VkExtent2D& getPresentationExtent() const = 0;

    virtual void cleanup() = 0;
    virtual void cleanupSwapchain() = 0;
};

class ShaderLibraryProviderInterface : virtual public VulkanProviderInterface, public CoreLink
{
 public:
    virtual void init() = 0;
    virtual void cleanup() = 0;

    virtual VulkanProviderType getProviderTypeBitmask() const override {return(VulkanProviderTypeEnum::SHADER_LIBRARY_BIT);}

    virtual void registerShaderModule(const std::string& aModuleId, const VkShaderModule& aModule) = 0;
    virtual VkShaderModule unregisterShaderModule(const std::string& aModuleId) = 0;
    virtual VkShaderModule getShaderModule(const std::string& aModuleId) const = 0;
};

class ComputeProviderInterface : virtual public VulkanProviderInterface, public CoreLink
{
 public:
   virtual void init() = 0;
   virtual void cleanup() = 0;

   virtual VulkanProviderType getProviderTypeBitmask() const override {return(VulkanProviderTypeEnum::COMPUTE_BIT);}

   virtual void registerComputeStage(const std::string& aStageId, const vkutils::ComputeStage& aComputeStage) = 0;
   virtual void unregisterComputeStage(const std::string& aStageId) = 0;
   virtual const vkutils::ComputeStage& getComputeStage(const std::string& aStageId) const = 0;
};

class RenderProviderInterface : virtual public VulkanProviderInterface, public CoreLink
{
 public:
    virtual void init() = 0;
    virtual void cleanup() = 0;
    virtual void cleanupSwapchainDependents() = 0;

    virtual VulkanProviderType getProviderTypeBitmask() const override {return(VulkanProviderTypeEnum::GRAPHICS_BIT);}

    virtual void initRenderPipeline() = 0;
    virtual void initFramebuffers() = 0;
    virtual void initCommands() = 0;
    virtual void initSync() = 0;

    virtual void resetRenderSetup() = 0;
    
    virtual size_t getFrameIndex() const = 0;
    virtual vkutils::VulkanRenderPipeline getRenderPipeline() const = 0;
};


class VulkanAppInterface
{
 public:
    virtual ~VulkanAppInterface() = default;

    // Virtual functions for retreving information about the setup of the applications vulkan 
    // instance and devices. Overriding these functions in derived classes allow modifying the 
    // Vulkan setup with minimal amounts of new or re-written logic. 

    /** Returns the Application information struct for the application */
    virtual const VkApplicationInfo& getAppInfo() const = 0;

    virtual const std::vector<std::string>& getRequiredValidationLayers() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedValidationLayers() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequiredInstanceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedInstanceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequiredDeviceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
    virtual const std::vector<std::string>& getRequestedDeviceExtensions() const {const static std::vector<std::string> sEmptySet; return(sEmptySet);};
};

#endif