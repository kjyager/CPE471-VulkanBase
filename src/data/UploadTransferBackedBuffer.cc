#include "UploadTransferBackedBuffer.h"
#include "vkutils/VmaHost.h"
#include <iostream>
#include <cstring>

void UploadTransferBackedBuffer::initDevice(const VulkanDeviceBundle& aDeviceBundle){
    if(aDeviceBundle.isValid() && mCurrentDevice != aDeviceBundle){
        freeAndReset();
        mCurrentDevice = VulkanDeviceHandlePair(aDeviceBundle);
    }
    
    if(!mCurrentDevice.isValid()){
        throw std::runtime_error("DualTransferBuffer could not be initialized due to having an invalid device!");
    }
    
    if(mResidentBuffer != VK_NULL_HANDLE){
        freeAndReset();
    }
}

void UploadTransferBackedBuffer::recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) {
    VkBufferCopy copyRegion = {0U, 0U, mCurrentBufferSize};
    vkCmdCopyBuffer(aCmdBuffer, mStagingBuffer, mResidentBuffer, 1, &copyRegion);
    mAwaitingUpload = false;
}

void UploadTransferBackedBuffer::freeStagingBuffer(){
    if(mStagingBuffer != VK_NULL_HANDLE){
        VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);
        vmaDestroyBuffer(allocator, mStagingBuffer, mStagingAllocation);

        mStagingBuffer = VK_NULL_HANDLE;
        mStagingAllocation = VK_NULL_HANDLE;
        mStagingAllocInfo = {};
    }
}

void UploadTransferBackedBuffer::freeAndReset(){
    freeStagingBuffer();

    if(mResidentBuffer != VK_NULL_HANDLE){
        VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);
        vmaDestroyBuffer(allocator, mResidentBuffer, mResidentAllocation);

        mResidentBuffer = VK_NULL_HANDLE;
        mResidentAllocation = VK_NULL_HANDLE;
        mResidentAllocInfo = {};
    }

    mCurrentBufferSize = 0U;
}

void UploadTransferBackedBuffer::createStagingBuffer(VkDeviceSize aRequiredSize, const VkBufferCreateInfo& aBufferInfo, const VmaAllocationCreateInfo& aAllocInfo){
    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    VkBufferCreateInfo bufferInfo;
    {
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = aRequiredSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;
    }

    VmaAllocationCreateInfo allocInfo = {};
    {
        allocInfo.flags = 0;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    if(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mStagingBuffer, &mStagingAllocation, &mStagingAllocInfo) != VK_SUCCESS){
        throw std::runtime_error("VMA based creation of AoS_ParticleBuffer staging buffer failed!");
    }

    mCurrentBufferSize = aRequiredSize;

    assert(mCurrentBufferSize <= mStagingAllocInfo.size && (mStagingAllocInfo.size - mCurrentBufferSize) <= 128);
}

void UploadTransferBackedBuffer::createResidentBuffer(VkDeviceSize aRequiredSize, const VkBufferCreateInfo& aBufferInfo, const VmaAllocationCreateInfo& aAllocInfo){
    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    VkBufferCreateInfo bufferInfo;
    {
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = aRequiredSize;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;
    }

    VmaAllocationCreateInfo allocInfo = {};
    {
        allocInfo.flags = 0;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    if(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mResidentBuffer, &mResidentAllocation, &mResidentAllocInfo) != VK_SUCCESS){
        throw std::runtime_error("VMA based creation of upload transfer staging buffer failed!");
    }

    assert(mCurrentBufferSize == aRequiredSize);

    assert(mCurrentBufferSize <= mResidentAllocInfo.size && (mResidentAllocInfo.size - mCurrentBufferSize) <= 128);
}

void UploadTransferBackedBuffer::prepareBuffersForUploadStaging(size_t aDataSize){
    if(!mCurrentDevice.isValid()){
        throw std::runtime_error("UploadTransferBackedBuffer used with null device!");
    }

    bool noBuffers = mStagingBuffer == VK_NULL_HANDLE && mResidentBuffer == VK_NULL_HANDLE;

    if(noBuffers || mCurrentBufferSize != aDataSize){
        freeAndReset();
        createStagingBuffer(aDataSize);
        createResidentBuffer(aDataSize);
    }else if(mStagingBuffer == VK_NULL_HANDLE){
        createStagingBuffer(aDataSize);
    }
    else if(mResidentBuffer == VK_NULL_HANDLE){
        createResidentBuffer(aDataSize);
    }
}

void UploadTransferBackedBuffer::stageDataForUpload(const uint8_t* aData, size_t aDataSize){
    prepareBuffersForUploadStaging(aDataSize);
    assert(mCurrentBufferSize == aDataSize);

    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    void* rawptr = nullptr;
    VkResult mapResult = vmaMapMemory(allocator, mStagingAllocation, &rawptr);

    if(mapResult == VK_SUCCESS && rawptr != nullptr){
        memcpy(rawptr, aData, mCurrentBufferSize);
        vmaUnmapMemory(allocator, mStagingAllocation);
        rawptr = nullptr;
    }else{
        throw std::runtime_error("UploadTransferBackedBuffer: Mapping to staging buffer failed!");
    }

    mAwaitingUpload = true;
}