#ifndef DEVICE_SYNCED_BUFFER_H_
#define DEVICE_SYNCED_BUFFER_H_

#include "utils/common.h"
#include "vkutils/VulkanDevices.h"
#include <vulkan/vulkan.h>

enum DeviceSyncStateEnum{
    DEVICE_EMPTY,
    DEVICE_OUT_OF_SYNC,
    DEVICE_IN_SYNC,
    CPU_DATA_FLUSHED
};

class DeviceSyncedBuffer
{
 public:

    virtual DeviceSyncStateEnum getDeviceSyncState() const = 0;
    virtual void updateDevice(VulkanDeviceHandlePair aDevicePair = {}) = 0;
    virtual VulkanDeviceHandlePair getCurrentDevice() const = 0;

    virtual const VkBuffer& getBuffer() const = 0;
    virtual const VkBuffer& handle() const {return(getBuffer());}

    virtual void freeBuffer() = 0;

 protected:

    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) = 0;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) = 0;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) = 0;

};

#endif 