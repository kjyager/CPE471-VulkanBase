#include "VulkanDevices.h"
#include <set>
#include <algorithm>

QueueFamily::QueueFamily(const VkQueueFamilyProperties& aFamily, uint32_t aIndex) 
: mIndex(aIndex),
  mCount(aFamily.queueCount),
  mFlags(aFamily.queueFlags),
  mMinImageTransferGranularity(aFamily.minImageTransferGranularity),
  mTimeStampValidBits(aFamily.timestampValidBits),
  mGraphics(aFamily.queueFlags | VK_QUEUE_GRAPHICS_BIT),
  mCompute(aFamily.queueFlags | VK_QUEUE_COMPUTE_BIT),
  mTransfer(aFamily.queueFlags | VK_QUEUE_TRANSFER_BIT),
  mSparseBinding(aFamily.queueFlags | VK_QUEUE_SPARSE_BINDING_BIT),
  mProtected(aFamily.queueFlags | VK_QUEUE_PROTECTED_BIT)
{}

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice aDevice) : mHandle(aDevice) {
    vkGetPhysicalDeviceProperties(aDevice, &mProperties);
    vkGetPhysicalDeviceFeatures(aDevice, &mFeatures);
    _initExtensionProps();
    _initQueueFamilies();
}

void VulkanPhysicalDevice::_initExtensionProps(){
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(mHandle, nullptr, &extensionCount, nullptr);
    mAvailableExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(mHandle, nullptr, &extensionCount, mAvailableExtensions.data());
}
void VulkanPhysicalDevice::_initQueueFamilies(){
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &queueCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &queueCount, queueProperties.data());

    for(unsigned int familyIdx = 0; familyIdx < queueProperties.size(); ++familyIdx){
        mQueueFamilies.emplace_back(queueProperties[familyIdx], familyIdx);
        const QueueFamily& queueFamily = mQueueFamilies.back();

        if(!coreFeaturesIdx && mQueueFamilies.back().hasCoreQueueSupport())
            coreFeaturesIdx = familyIdx;
        if(!mGraphicsIdx && queueFamily.mGraphics)
            mGraphicsIdx = familyIdx;
        if(!mComputeIdx && queueFamily.mCompute)
            mComputeIdx = familyIdx;
        if(!mTransferIdx && queueFamily.mTransfer)
            mTransferIdx = familyIdx;
        if(!mProtectedIdx && queueFamily.mProtected)
            mProtectedIdx = familyIdx;
        if(!mSparseBindIdx && queueFamily.mSparseBinding)
            mSparseBindIdx = familyIdx;
    }
}
SwapChainSupportInfo VulkanPhysicalDevice::getSwapChainSupportInfo(const VkSurfaceKHR aSurface) const{
    SwapChainSupportInfo info;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mHandle, aSurface, &info.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mHandle, aSurface, &formatCount, nullptr);
    info.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mHandle, aSurface, &formatCount, info.formats.data());

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mHandle, aSurface, &modeCount, nullptr);
    info.presentation_modes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mHandle, aSurface, &modeCount, info.presentation_modes.data());

    return(info);
}

opt::optional<uint32_t> VulkanPhysicalDevice::getPresentableQueueIndex(const VkSurfaceKHR aSurface) const{
    for(const QueueFamily& fam : mQueueFamilies){
        VkBool32 support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(mHandle, fam.mIndex, aSurface, &support);
        if(support) return(opt::optional<uint32_t>(fam.mIndex));
    }
    return(opt::optional<uint32_t>());
}

VulkanLogicalDevice VulkanPhysicalDevice::createLogicalDevice(VkQueueFlags aQueues, const std::vector<const char*>& aExtensions, VkSurfaceKHR aSurface) const{
    std::set<uint32_t> queueFamilyIndices;
    if(aQueues | VK_QUEUE_GRAPHICS_BIT && mGraphicsIdx) queueFamilyIndices.emplace(*mGraphicsIdx);
    if(aQueues | VK_QUEUE_COMPUTE_BIT && mComputeIdx) queueFamilyIndices.emplace(*mComputeIdx);
    if(aQueues | VK_QUEUE_TRANSFER_BIT && mTransferIdx) queueFamilyIndices.emplace(*mTransferIdx);
    if(aQueues | VK_QUEUE_PROTECTED_BIT && mProtectedIdx) queueFamilyIndices.emplace(*mProtectedIdx);
    if(aQueues | VK_QUEUE_SPARSE_BINDING_BIT && mSparseBindIdx) queueFamilyIndices.emplace(*mSparseBindIdx);
    
    opt::optional<uint32_t> presentationIdx;
    if(aSurface != VK_NULL_HANDLE){
        presentationIdx = getPresentableQueueIndex(aSurface); 
        if(presentationIdx){
            queueFamilyIndices.emplace(*presentationIdx);
        }else{
            throw std::runtime_error("Unable to get presentation queue during device creation!");
        }
    }

    float priorities = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyIndices.size());
    std::set<uint32_t>::const_iterator famIter = queueFamilyIndices.begin();
    size_t i = 0;
    while(famIter != queueFamilyIndices.end()){
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].pNext = nullptr;
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].queueFamilyIndex = *famIter;
        queueCreateInfos[i].pQueuePriorities = &priorities;
        ++famIter; ++i;
    }

    VkDeviceCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pEnabledFeatures = {};
        createInfo.flags = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.enabledLayerCount = 0;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.ppEnabledExtensionNames = aExtensions.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(aExtensions.size());
    }

    VkDevice deviceHandle = VK_NULL_HANDLE;
    if(vkCreateDevice(mHandle, &createInfo, nullptr, &deviceHandle) != VK_SUCCESS){
        throw std::runtime_error("Failed to create device!");
    }

    VulkanLogicalDevice device = VulkanLogicalDevice(deviceHandle);

    if(mGraphicsIdx) vkGetDeviceQueue(deviceHandle, *mGraphicsIdx, 0, &device.mGraphicsQueue);
    if(mComputeIdx) vkGetDeviceQueue(deviceHandle, *mComputeIdx, 0, &device.mComputeQueue);
    if(mTransferIdx) vkGetDeviceQueue(deviceHandle, *mTransferIdx, 0, &device.mTransferQueue);
    if(presentationIdx) vkGetDeviceQueue(deviceHandle, *presentationIdx, 0, &device.mPresentationQueue);
    if(mProtectedIdx) vkGetDeviceQueue(deviceHandle, *mProtectedIdx, 0, &device.mProtectedQueue);
    if(mSparseBindIdx) vkGetDeviceQueue(deviceHandle, *mSparseBindIdx, 0, &device.mSparseBindingQueue);

    return(device);
}

VulkanPhysicalDeviceEnumeration::VulkanPhysicalDeviceEnumeration(const std::vector<VkPhysicalDevice>& aDevices) {
    base_vector::resize(aDevices.size());
    for(VkPhysicalDevice device : aDevices){
        base_vector::emplace_back(device);
    }
}