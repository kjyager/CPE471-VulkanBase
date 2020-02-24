#ifndef KJY_UPLOAD_TRANSFER_BACKED_BUFFER_H_
#define KJY_UPLOAD_TRANSFER_BACKED_BUFFER_H_
#include <vk_mem_alloc.h>
#include "SyncedBuffer.h"

/** Data agnostic vulkan buffer which sits in local memory and is accessed for input and output
 * through a host visible staging buffer and memory mapping */
class UploadTransferBackedBuffer : public virtual UploadTransferBackedBufferInterface
{
 public:

    UploadTransferBackedBuffer(VkBufferUsageFlags aBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : mResidentUsageFlags(aBufferUsage){}
    UploadTransferBackedBuffer(
        const VulkanDeviceBundle& aDeviceBundle,
        VkBufferUsageFlags aBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    ) : mResidentUsageFlags(aBufferUsage) {initDevice(aDeviceBundle);}

    virtual void initDevice(const VulkanDeviceBundle& aDeviceBundle);

    virtual void recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) override;

    virtual void stageDataForUpload(const uint8_t* aData, size_t aDataSize);

    virtual bool awaitingUploadTransfer() const {return(mAwaitingUpload);}

    virtual const VkBuffer& getBuffer() const override {return(mResidentBuffer);}
    virtual size_t getBufferSize() const override {return(mCurrentBufferSize);}

    virtual void freeStagingBuffer();

    virtual void freeAndReset(); 

 protected:

    virtual void createStagingBuffer(VkDeviceSize aRequiredSize, const VkBufferCreateInfo& aBufferInfo = {}, const VmaAllocationCreateInfo& aAllocInfo = {});
    virtual void createResidentBuffer(VkDeviceSize aRequiredSize, const VkBufferCreateInfo& aBufferInfo = {}, const VmaAllocationCreateInfo& aAllocInfo = {});

    virtual void prepareBuffersForUploadStaging(size_t aDataSize);

    bool mAwaitingUpload = false;

    VulkanDeviceHandlePair mCurrentDevice {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkBufferUsageFlags mResidentUsageFlags;

    VkBuffer mResidentBuffer = VK_NULL_HANDLE;
    VkBuffer mStagingBuffer = VK_NULL_HANDLE;
    VkDeviceSize mCurrentBufferSize = 0U;

    VmaAllocation mResidentAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mResidentAllocInfo = {};
    VmaAllocation mStagingAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mStagingAllocInfo = {};
};

#endif