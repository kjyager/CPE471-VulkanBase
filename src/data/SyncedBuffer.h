#ifndef KJY_SYNCED_BUFFER_H_
#define KJY_SYNCED_BUFFER_H_

#include "utils/common.h"
#include "vkutils/VulkanDevices.h"
#include <vulkan/vulkan.h>


enum DeviceSyncStateEnum{
    DEVICE_EMPTY,
    DEVICE_OUT_OF_SYNC,
    DEVICE_IN_SYNC,
    CPU_DATA_FLUSHED
};

class SyncedBufferInterface
{
 public:

    virtual DeviceSyncStateEnum getDeviceSyncState() const = 0;
    virtual VulkanDeviceHandlePair getCurrentDevice() const = 0;

    virtual size_t getBufferSize() const = 0;

    virtual const VkBuffer& getBuffer() const = 0;
    virtual const VkBuffer& handle() const {return(getBuffer());}

    /** Free all resources on both host and device, and reset state 
     * of the object in all ways except the device to which it targets.
     */
    virtual void freeAndReset() = 0;
};

class DirectlySyncedBufferInterface : virtual public SyncedBufferInterface
{
 public:
    virtual void updateDevice() = 0;

 protected:
    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) = 0;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) = 0;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) = 0;

};

class TransferBackedBufferBase
{
 public:
    virtual size_t getBufferSize() const = 0;

    virtual const VkBuffer& getBuffer() const = 0;
    virtual const VkBuffer& handle() const {return(getBuffer());}

    /** Free all resources on both host and device, and reset state 
     * of the object in all ways except the device to which it targets.
     */
    virtual void freeAndReset() = 0;
};

class UploadTransferBackedBufferInterface : public virtual TransferBackedBufferBase
{
 public:
    /// Records transfer command for uploading data from the staging buffer into device local memory into 'aCmdBuffer'
    virtual void recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) = 0;
};


class DownloadTransferBackedBufferInterface : virtual public TransferBackedBufferBase
{
 public:
    /// Records transfer command for downloading data from device local memory into the staging buffer. Records into 'aCmdBuffer'
    virtual void recordDownloadTransferCommand(const VkCommandBuffer& aCmdBuffer) = 0;
};

/// Transfer based buffer supporting both upload and download from device local memory. 
class DualTransferBackedBufferInterface : virtual public UploadTransferBackedBufferInterface, virtual public DownloadTransferBackedBufferInterface{};

#endif 