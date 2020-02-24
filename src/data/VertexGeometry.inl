#include "VertexGeometry.h"

template<typename VertexType> 
void HostVisVertexAttrBuffer<VertexType>::updateDevice(){
    if(!mCurrentDevice.isValid()){
        throw std::runtime_error("Attempting to updateDevice() from vertex attribute buffer with no associated device!");
    }

    setupDeviceUpload(mCurrentDevice);
    uploadToDevice(mCurrentDevice);
    finalizeDeviceUpload(mCurrentDevice);
}

template<typename VertexType> 
void HostVisVertexAttrBuffer<VertexType>::updateDevice(const VulkanDeviceBundle& aDeviceBundle){
    if(aDeviceBundle.isValid() && aDeviceBundle != mCurrentDevice){
        _cleanup();
        mCurrentDevice = VulkanDeviceHandlePair(aDeviceBundle); 
    }

    updateDevice();
}


template<typename VertexType>
void HostVisVertexAttrBuffer<VertexType>::setupDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    VkDeviceSize requiredSize = sizeof(VertexType) * mCpuVertexData.size();
    
    if(mDeviceSyncState == DEVICE_EMPTY || requiredSize != mCurrentBufferSize){
        VkBufferCreateInfo createInfo;
        {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.size = requiredSize;
            createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0U;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        if(vkCreateBuffer(aDevicePair.device, &createInfo, nullptr, &mVertexBuffer) != VK_SUCCESS){
            throw std::runtime_error("Failed to create vertex buffer!"); 
        }
    }

}

template<typename VertexType>
void HostVisVertexAttrBuffer<VertexType>::uploadToDevice(VulkanDeviceHandlePair aDevicePair){
    VkDeviceSize requiredSize = sizeof(VertexType) * mCpuVertexData.size();
    
    if(mDeviceSyncState == DEVICE_EMPTY || requiredSize != mCurrentBufferSize){
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(aDevicePair.device, mVertexBuffer, &memRequirements);

        VkPhysicalDeviceMemoryProperties memoryProps;
        vkGetPhysicalDeviceMemoryProperties(aDevicePair.physicalDevice, &memoryProps);

        uint32_t memTypeIndex = VK_MAX_MEMORY_TYPES; 
        for(uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i){
            if(memRequirements.memoryTypeBits & (1 << i) && memoryProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){
                memTypeIndex = i;
                break;
            }
        }
        if(memTypeIndex == VK_MAX_MEMORY_TYPES){
            throw std::runtime_error("No compatible memory type could be found for uploading vertex attribute buffer to device!");
        }

        VkMemoryAllocateInfo allocInfo;
        {
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = memTypeIndex;
        }

        _mCurrentDeviceAllocSize = memRequirements.size;

        if(vkAllocateMemory(aDevicePair.device, &allocInfo, nullptr, &mVertexBufferMemory) != VK_SUCCESS){
            throw std::runtime_error("Failed to allocate memory for vertex attribute buffer!");
        }

        vkBindBufferMemory(aDevicePair.device, mVertexBuffer, mVertexBufferMemory, 0);
        mCurrentBufferSize = requiredSize;
    }

    void* mappedPtr = nullptr;
    VkResult mapResult = vkMapMemory(aDevicePair.device, mVertexBufferMemory, 0, _mCurrentDeviceAllocSize , 0, &mappedPtr);
    if(mapResult != VK_SUCCESS || mappedPtr == nullptr) throw std::runtime_error("Failed to map memory during vertex attribute buffer upload!");
    {
        memcpy(mappedPtr, mCpuVertexData.data(), mCurrentBufferSize);

        VkMappedMemoryRange mappedMemRange;
        {
            mappedMemRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemRange.pNext = nullptr;
            mappedMemRange.memory = mVertexBufferMemory;
            mappedMemRange.offset = 0;
            mappedMemRange.size = _mCurrentDeviceAllocSize;
        }
        if(vkFlushMappedMemoryRanges(aDevicePair.device, 1, &mappedMemRange) != VK_SUCCESS){
            throw std::runtime_error("Failed to flush mapped memory during vertex attribute buffer upload!");
        }
    }vkUnmapMemory(aDevicePair.device, mVertexBufferMemory); mappedPtr = nullptr;

}

template<typename VertexType>
void HostVisVertexAttrBuffer<VertexType>::finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    mDeviceSyncState = DEVICE_IN_SYNC;
}

template<typename VertexType>
void HostVisVertexAttrBuffer<VertexType>::_cleanup(){
    if(mVertexBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(mCurrentDevice.device, mVertexBuffer, nullptr);
        mVertexBuffer = VK_NULL_HANDLE;
    }
    if(mVertexBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(mCurrentDevice.device, mVertexBufferMemory, nullptr);
        mVertexBufferMemory = VK_NULL_HANDLE;
    }
    mCurrentBufferSize = 0U;
    _mCurrentDeviceAllocSize = 0U;
    mDeviceSyncState = DEVICE_EMPTY;
}


template<typename VertexType, typename IndexType>
IndexedVertexGeometry<VertexType, IndexType>::IndexedVertexGeometry(const VulkanDeviceBundle& aDeviceBundle) : mVertexBuffer(aDeviceBundle), mIndexBuffer(aDeviceBundle) {}

template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::setDevice(const VulkanDeviceBundle& aDeviceBundle){
    mVertexBuffer.initDevice(aDeviceBundle);
    mIndexBuffer.initDevice(aDeviceBundle);
}

template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::setVertices(const std::vector<VertexType>& aVertices){
    mVertexBuffer.stageDataForUpload(reinterpret_cast<const uint8_t*>(aVertices.data()), aVertices.size());
}

template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::setIndices(const std::vector<index_t>& aIndices){
    mIndexBuffer.stageDataForUpload(reinterpret_cast<const uint8_t*>(aIndices.data()), aIndices.size());
}

/// Records commands to upload both the index and attribute buffers to device local memory.
/// Commands are recorded into aCmdBuffer. 
template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) {
    mVertexBuffer.recordUploadTransferCommand(aCmdBuffer);
    mIndexBuffer.recordUploadTransferCommand(aCmdBuffer);
}

template<typename VertexType, typename IndexType>
size_t IndexedVertexGeometry<VertexType, IndexType>::getBufferSize() const {
    return(mVertexBuffer.getBufferSize() + mIndexBuffer.getBufferSize());
}

template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::freeStagingBuffer() {
    mVertexBuffer.freeStagingBuffer();
    mIndexBuffer.freeStagingBuffer();
}

template<typename VertexType, typename IndexType>
void IndexedVertexGeometry<VertexType, IndexType>::freeAndReset() {
    mVertexBuffer.freeAndReset();
    mIndexBuffer.freeAndReset();
}

template<typename VertexType, typename IndexType>
void MultiShapeGeometry<VertexType, IndexType>::recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) {
    if(super_t::mIndexBuffer.getBuffer() == VK_NULL_HANDLE)
        setIndices(mIndicesConcat);
    super_t::recordUploadTransferCommand(aCmdBuffer);
}