#include "VmaHost.h"

VmaAllocator VmaHost::_getAllocator(const VulkanDeviceHandlePair& aDevicePair){
    base_map_t::const_iterator finder = this->find(aDevicePair);
    if(finder == this->end()){
        VmaAllocator allocator = _createNewAllocator(aDevicePair);
        this->insert({aDevicePair, allocator});
        return(allocator);
    }else{
        return(finder->second);
    }
}

void VmaHost::_destroyAllocator(const VulkanDeviceHandlePair& aDevicePair){
    base_map_t::const_iterator finder = this->find(aDevicePair);
    if(finder != this->end()){
        vmaDestroyAllocator(finder->second);
        this->erase(finder);
    }
}

bool VmaHost::_allocatorExists(const VulkanDeviceHandlePair& aDevicePair){
    base_map_t::const_iterator finder = this->find(aDevicePair);
    return(finder != this->end());
}

VmaAllocator VmaHost::_createNewAllocator(const VulkanDeviceHandlePair& aDevicePair){
    VmaAllocatorCreateInfo createInfo = {};
    {
		createInfo.instance = _mInstance;
        createInfo.device = aDevicePair.device;
        createInfo.physicalDevice = aDevicePair.physicalDevice;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    }

    VmaAllocator allocator = nullptr;
    vmaCreateAllocator(&createInfo, &allocator);
    return(allocator);
}