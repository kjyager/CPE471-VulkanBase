#ifndef KJY_VMA_HOST_H_
#define KJY_VMA_HOST_H_
#include <unordered_map> 
#include <vk_mem_alloc.h>
#include "VulkanDevices.h"
#include <functional> 

template<>
struct std::hash<VulkanDeviceHandlePair>{
    size_t operator()(const VulkanDeviceHandlePair& aDevicePair) const noexcept{
        return(
            std::hash<VkDevice>()(aDevicePair.device)
            ^
            std::hash<VkPhysicalDevice>()(aDevicePair.physicalDevice)
        );
    }
};

class VmaHost : public std::unordered_map<VulkanDeviceHandlePair, VmaAllocator>
{
 public:

    using base_map_t = typename std::unordered_map<VulkanDeviceHandlePair, VmaAllocator>;

    ~VmaHost(){
        for(auto& entry : *this){
            vmaDestroyAllocator(entry.second);
        }
    }

    static VmaHost& getInstance(){
        static VmaHost instance;
        return(instance);
    }

	static void setVkInstance(VkInstance aVkInstance) {
		VmaHost::getInstance()._mInstance = aVkInstance;
	}

    static bool allocatorExists(const VulkanDeviceHandlePair& aDevicePair){
        return(VmaHost::getInstance()._allocatorExists(aDevicePair));
    }

    static VmaAllocator getAllocator(const VulkanDeviceHandlePair& aDevicePair){
        return(VmaHost::getInstance()._getAllocator(aDevicePair));
    }

    static void destroyAllocator(const VulkanDeviceHandlePair& aDevicePair){
        VmaHost::getInstance()._destroyAllocator(aDevicePair);
    }

    VmaHost(const VmaHost&) = delete;
    VmaHost& operator=(const VmaHost&) = delete;

 private:
    VmaHost(){}

    VmaAllocator _getAllocator(const VulkanDeviceHandlePair& aDevicePair);
    VmaAllocator _createNewAllocator(const VulkanDeviceHandlePair& aDevicePair);
    void _destroyAllocator(const VulkanDeviceHandlePair& aDevicePair);
    bool _allocatorExists(const VulkanDeviceHandlePair& aDevicePair);

	VkInstance _mInstance = VK_NULL_HANDLE;
};

#endif