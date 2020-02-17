#ifndef KJY_DUAL_TRANSFER_BACKED_BUFFER_H_
#define KJY_DUAL_TRANSFER_BACKED_BUFFER_H_
#include <vk_mem_alloc.h>
#include "SyncedBuffer.h"

/** Data agnostic vulkan buffer which sits in local memory and is accessed for input and output
 * through a host visible staging buffer and memory mapping */
class DualTransferBackedBuffer : public virtual DualTransferBackedBufferInterface
{
 public:

    DualTransferBackedBuffer(){}
    DualTransferBackedBuffer(const VulkanDeviceBundle& aDeviceBundle){initDevice(aDeviceBundle);}

    virtual void initDevice(const VulkanDeviceBundle& aDeviceBundle);

    virtual void recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) override;
    virtual void recordDownloadTransferCommand(const VkCommandBuffer& aCmdBuffer) override;

    virtual void stageDataForUpload(const uint8_t* aData, size_t aDataSize);
    virtual size_t copyDataFromStage(uint8_t* aDst, size_t aSizeLimit = std::numeric_limits<size_t>::max());

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

    VkBuffer mResidentBuffer = VK_NULL_HANDLE;
    VkBuffer mStagingBuffer = VK_NULL_HANDLE;
    VkDeviceSize mCurrentBufferSize = 0U;

    VmaAllocation mResidentAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mResidentAllocInfo = {};
    VmaAllocation mStagingAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mStagingAllocInfo = {};

};

#endif