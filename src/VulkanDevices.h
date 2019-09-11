#ifndef VULKAN_DEVICES_H_
#define VULKAN_DEVICES_H_
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <limits>

class QueueFamily
{
 public:
    QueueFamily(){}
    QueueFamily(const VkQueueFamilyProperties& aFamily, uint32_t aIndex);

    inline bool hasCoreQueueSupport() const {return(mGraphics && mCompute && mTransfer);}
    inline bool hasAllQueueSupport() const {return(mGraphics && mCompute && mTransfer && mSparseBinding && mProtected);}

    const uint32_t mIndex = std::numeric_limits<uint32_t>::max();
    const uint32_t mCount = 0;
    const VkQueueFlags mFlags = 0x0;
    const VkExtent3D mMinImageTransferGranularity = VkExtent3D();
    const uint32_t mTimeStampValidBits = 0;

    const bool mGraphics = false;
    const bool mCompute = false;
    const bool mTransfer = false;
    const bool mSparseBinding = false;
    const bool mProtected = false;
};

class VulkanDevice
{
 public:

    VulkanDevice(){}

    inline VkDevice handle() const {return(mHandle);}
    inline bool isValid() const {return(mHandle == VK_NULL_HANDLE);}

    VkQueue getGraphicsQueue() const {return(mGraphicsQueue);}
    VkQueue getComputeQueue() const {return(mComputeQueue);}
    VkQueue getTransferQueue() const {return(mTransferQueue);}
    VkQueue getSparseBindingQueue() const {return(mSparseBindingQueue);}
    VkQueue getProtectedQueue() const {return(mProtectedQueue);}
    VkQueue getPresentationQueue() const {return(mPresentationQueue);}

    operator VkDevice() const {return(mHandle);}

 protected:
    friend class VulkanPhysicalDevice;

    VulkanDevice(VkDevice aDevice) : mHandle(aDevice) {}

    VkDevice mHandle = VK_NULL_HANDLE;

    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mComputeQueue = VK_NULL_HANDLE;
    VkQueue mTransferQueue = VK_NULL_HANDLE;
    VkQueue mSparseBindingQueue = VK_NULL_HANDLE;
    VkQueue mProtectedQueue = VK_NULL_HANDLE;
    VkQueue mPresentationQueue = VK_NULL_HANDLE;
};

struct SwapChainSupportInfo
{
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentation_modes;
};

class VulkanPhysicalDevice
{
 public:
   VulkanPhysicalDevice(){}
   VulkanPhysicalDevice(VkPhysicalDevice aDevice);

   inline VkPhysicalDevice handle() const {return(mHandle);}

   SwapChainSupportInfo getSwapChainSupportInfo(const VkSurfaceKHR aSurface) const;

   std::optional<uint32_t> getPresentableQueueIndex(const VkSurfaceKHR aSurface) const;
   
   VulkanDevice createDevice(
      VkQueueFlags aQueues,
      const std::vector<const char*>& aExtensions = std::vector<const char*>(),
      VkSurfaceKHR aSurface = VK_NULL_HANDLE
   ) const;

   VulkanDevice createCoreDevice() const { return(createDevice(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)); }

   VulkanDevice createPresentableCoreDevice(VkSurfaceKHR aSurface, const std::vector<const char*>& aExtensions = std::vector<const char*>()) const {
      if(aSurface == VK_NULL_HANDLE) throw std::runtime_error("Attempted to create presentable core device with invalid surface handle!");
      return(createDevice(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, aExtensions, aSurface));
   }

   VkPhysicalDeviceProperties mProperites;
   VkPhysicalDeviceFeatures mFeatures;
   std::vector<QueueFamily> mQueueFamilies;
   std::vector<VkExtensionProperties> mAvailableExtensions;

   std::optional<uint32_t> mGraphicsIdx;
   std::optional<uint32_t> mComputeIdx;
   std::optional<uint32_t> mTransferIdx;
   std::optional<uint32_t> mProtectedIdx;
   std::optional<uint32_t> mSparseBindIdx;

   // Index of queue supporting graphics, compute, transfer, and presentation
   std::optional<uint32_t> coreFeaturesIdx; 

   operator VkPhysicalDevice() const {return(mHandle);}

 protected:
   void _initExtensionProps();
   void _initQueueFamilies();

   VkPhysicalDevice mHandle = VK_NULL_HANDLE;
};

class VulkanPhysicalDeviceEnumeration : public std::vector<VulkanPhysicalDevice>
{
 public:
    using base_vector = std::vector<VulkanPhysicalDevice>;

    VulkanPhysicalDeviceEnumeration(){}
    VulkanPhysicalDeviceEnumeration(const std::vector<VkPhysicalDevice>& aDevices);

    using base_vector::operator[];
};

#endif