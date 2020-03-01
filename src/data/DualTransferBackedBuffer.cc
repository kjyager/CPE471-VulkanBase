#include "DualTransferBackedBuffer.h"
#include "vkutils/VmaHost.h"
#include <iostream>
#include <algorithm>
#include <cstring>

void DualTransferBackedBuffer::recordDownloadTransferCommand(const VkCommandBuffer& aCmdBuffer) {
    VkBufferCopy copyRegion = {0U, 0U, mCurrentBufferSize};
    vkCmdCopyBuffer(aCmdBuffer, mResidentBuffer, mStagingBuffer, 1, &copyRegion);
}

size_t DualTransferBackedBuffer::copyDataFromStage(uint8_t* aDst, size_t aSizeLimit){
    if(!mCurrentDevice.isValid()){
        throw std::runtime_error("DualTransferBackedBuffer used with null device!");
    }
    if(mResidentBuffer == VK_NULL_HANDLE){
        throw std::runtime_error("DualTransferBackedBuffer::copyDataFromStage() was called, but buffers were not created");
    }
    if(mStagingBuffer == VK_NULL_HANDLE){
        createStagingBuffer(mCurrentBufferSize);
    }

    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    size_t copySize = std::min<size_t>(aSizeLimit, mCurrentBufferSize);

    void* rawptr = nullptr;
    VkResult mapResult = vmaMapMemory(allocator, mStagingAllocation, &rawptr);

    if(mapResult == VK_SUCCESS && rawptr != nullptr){
        memcpy(aDst, rawptr, copySize);
        vmaUnmapMemory(allocator, mStagingAllocation);
        rawptr = nullptr;
    }else{
        throw std::runtime_error("DualTransferBackedBuffer: Mapping to staging buffer failed!");
    }

    return(copySize);
}
