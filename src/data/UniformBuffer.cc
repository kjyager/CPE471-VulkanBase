#include "UniformBuffer.h"
#include <iostream>
#include <cstring>

UniformBuffer::UniformBuffer(const VulkanDeviceBundle& aDeviceBundle){
    if(aDeviceBundle.isValid()){
            mCurrentDevice = VulkanDeviceHandlePair(aDeviceBundle);
            mBufferAlignmentSize = aDeviceBundle.physicalDevice.mProperites.limits.minUniformBufferOffsetAlignment;
    }
}

void UniformBuffer::bindUniformData(uint32_t aBindPoint, UniformDataInterfacePtr aUniformData, VkShaderStageFlags aStageFlags){
    auto findExisting = mBoundUniformData.find(aBindPoint);
    if(findExisting != mBoundUniformData.end()){
        mBoundUniformData.erase(findExisting);
    }

    if(aUniformData == nullptr) return;

    mBoundUniformData[aBindPoint] = BoundUniformData{
        aUniformData,
        {
            /* binding = */ aBindPoint,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount = */ 1,
            /* stageFlags = */ aStageFlags,
            /* pImmutableSamplers = */ nullptr
        }
    };

    mDeviceSyncState = DEVICE_OUT_OF_SYNC;
    mLayoutOutOfDate = true;
}

bool UniformBuffer::isBoundDataDirty() const {
    bool result = false;
    for(const std::pair<const uint32_t, BoundUniformData>& boundData : mBoundUniformData){
        result |= boundData.second.mDataInterface->isDataDirty();
    }
    return(result);
}

void UniformBuffer::pollBoundData() {
    if(isBoundDataDirty() && mDeviceSyncState == DEVICE_IN_SYNC) 
        mDeviceSyncState = DEVICE_OUT_OF_SYNC;
}

DeviceSyncStateEnum UniformBuffer::getDeviceSyncState() const {
    if(mDeviceSyncState != DEVICE_IN_SYNC)
        return(mDeviceSyncState);
    if(isBoundDataDirty())
        return(DEVICE_OUT_OF_SYNC);
    return(mDeviceSyncState);
}

void UniformBuffer::updateDevice(const VulkanDeviceBundle& aDeviceBundle){
    if(aDeviceBundle.isValid() && aDeviceBundle != mCurrentDevice){
        _cleanup();
        mCurrentDevice = VulkanDeviceHandlePair(aDeviceBundle); 
        mBufferAlignmentSize = aDeviceBundle.physicalDevice.mProperites.limits.minUniformBufferOffsetAlignment;
    }

    if(!mCurrentDevice.isValid()){
        throw std::runtime_error("Attempting to updateDevice() from uniform buffer with no associated device!");
    }

    if(mDeviceSyncState == DEVICE_OUT_OF_SYNC || mDeviceSyncState == DEVICE_EMPTY || isBoundDataDirty()){
        setupDeviceUpload(mCurrentDevice);
        uploadToDevice(mCurrentDevice);
        finalizeDeviceUpload(mCurrentDevice);
    }
}

size_t UniformBuffer::getBoundDataOffset(uint32_t aBindPoint) const{
    // Setup accumulator for offset and the start and end iterators which cover each bound data
    // object prior to 'aBindPoint' 
    size_t offsetAccum = 0;
    auto iter = mBoundUniformData.begin();
    auto lowerBoundIter = mBoundUniformData.lower_bound(aBindPoint);

    // Verify that 'aBindPoint' maps to a valid element of the map
    assert(lowerBoundIter != mBoundUniformData.end() && lowerBoundIter->first == aBindPoint);

    for(/*no-op*/; iter != lowerBoundIter; ++iter){
        offsetAccum += iter->second.mDataInterface->getPaddedDataSize(mBufferAlignmentSize);
    }

    return(offsetAccum);
}

std::vector<VkDescriptorBufferInfo> UniformBuffer::getDescriptorBufferInfos() const {
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(mBoundUniformData.size());

    size_t offset = 0;
    for(const std::pair<uint32_t, BoundUniformData>& boundData : mBoundUniformData){
        bufferInfos.emplace_back(VkDescriptorBufferInfo{
            /* buffer = */ mUniformBuffer,
            /* offset = */ offset,
            /* range = */ boundData.second.mDataInterface->getDataSize()
        });
        offset += boundData.second.mDataInterface->getPaddedDataSize(mBufferAlignmentSize);
    }
    return(bufferInfos);
}

std::vector<uint32_t> UniformBuffer::getBoundPoints() const {
    std::vector<uint32_t> bindPoints;
    bindPoints.reserve(mBoundUniformData.size());
    for(const std::pair<uint32_t, BoundUniformData>& boundData: mBoundUniformData){
        bindPoints.emplace_back(boundData.first);
    }
    return(bindPoints);
}

void UniformBuffer::createDescriptorSetLayout(){
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(mBoundUniformData.size());
    for(const std::pair<uint32_t, BoundUniformData>& boundData : mBoundUniformData){
        bindings.emplace_back(boundData.second.mLayoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = bindings.size();
        createInfo.pBindings = bindings.data();
    }

    if(vkCreateDescriptorSetLayout(mCurrentDevice.device, &createInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create descriptor set layout for uniform buffer!");
    }
}

void UniformBuffer::createUniformBuffer(){
    size_t requiredBufferSize = 0;
    for(const std::pair<uint32_t, BoundUniformData>& boundData : mBoundUniformData){
        requiredBufferSize += boundData.second.mDataInterface->getPaddedDataSize(mBufferAlignmentSize);
    }

    if(requiredBufferSize == 0){
        throw std::runtime_error(
            "Required size of uniform buffer is zero, and buffer creation cannot take place.\n"\
            "Verify that UniformBuffer::updateDevice() is not called before some uniform data is bound."
        );
    }

    if(mDeviceSyncState == DEVICE_EMPTY || requiredBufferSize != mCurrentBufferSize){
        VkBufferCreateInfo createInfo;
        {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.size = requiredBufferSize;
            createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0U;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        if(vkCreateBuffer(mCurrentDevice.device, &createInfo, nullptr, &mUniformBuffer) != VK_SUCCESS){
            throw std::runtime_error("Failed to create uniform buffer!"); 
        }
        mCurrentBufferSize = requiredBufferSize;
    }
}

void UniformBuffer::setupDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    if(mUniformBuffer == VK_NULL_HANDLE)
        createUniformBuffer();

    if(mLayoutOutOfDate){
        createDescriptorSetLayout();
    }
}

void UniformBuffer::uploadToDevice(VulkanDeviceHandlePair aDevicePair){
    if(mDeviceSyncState == DEVICE_EMPTY){
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(aDevicePair.device, mUniformBuffer, &memRequirements);

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
            throw std::runtime_error("No compatible memory type could be found for uploading uniform buffer to device!");
        }

        VkMemoryAllocateInfo allocInfo;
        {
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = memTypeIndex;
        }

        _mCurrentDeviceAllocSize = memRequirements.size;

        if(vkAllocateMemory(aDevicePair.device, &allocInfo, nullptr, &mUniformBufferMemory) != VK_SUCCESS){
            throw std::runtime_error("Failed to allocate memory for uniform buffer!");
        }

        vkBindBufferMemory(aDevicePair.device, mUniformBuffer, mUniformBufferMemory, 0);
    }

    void* mappedPtr = nullptr;
    VkResult mapResult = vkMapMemory(aDevicePair.device, mUniformBufferMemory, 0, _mCurrentDeviceAllocSize , 0, &mappedPtr);
    if(mapResult != VK_SUCCESS || mappedPtr == nullptr) throw std::runtime_error("Failed to map memory during uniform buffer upload!");
    {
        size_t offset = 0;
        uint8_t* mappedStart = reinterpret_cast<uint8_t*>(mappedPtr);
        for(const std::pair<uint32_t, BoundUniformData>& boundData : mBoundUniformData){
            uint8_t* start = mappedStart + offset;
            const uint8_t* data = boundData.second.mDataInterface->getData();
            size_t cpySize = boundData.second.mDataInterface->getDataSize();

            memcpy(start, data, cpySize);

            offset += boundData.second.mDataInterface->getPaddedDataSize(mBufferAlignmentSize);
            boundData.second.mDataInterface->flagAsClean();
        }

        VkMappedMemoryRange mappedMemRange;
        {
            mappedMemRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemRange.pNext = nullptr;
            mappedMemRange.memory = mUniformBufferMemory;
            mappedMemRange.offset = 0;
            mappedMemRange.size = _mCurrentDeviceAllocSize;
        }
        if(vkFlushMappedMemoryRanges(aDevicePair.device, 1, &mappedMemRange) != VK_SUCCESS){
            throw std::runtime_error("Failed to flush mapped memory during uniform buffer upload!");
        }
    }vkUnmapMemory(aDevicePair.device, mUniformBufferMemory); mappedPtr = nullptr;
}

void UniformBuffer::finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    mDeviceSyncState = DEVICE_IN_SYNC;
    mLayoutOutOfDate = false;
}

void UniformBuffer::_cleanup(){
    if(mUniformBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(mCurrentDevice.device, mUniformBuffer, nullptr);
        mUniformBuffer = VK_NULL_HANDLE;
    }
    if(mUniformBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(mCurrentDevice.device, mUniformBufferMemory, nullptr);
        mUniformBufferMemory = VK_NULL_HANDLE;
    }

    if(mDescriptorSetLayout != VK_NULL_HANDLE){
        vkDestroyDescriptorSetLayout(mCurrentDevice.device, mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout = VK_NULL_HANDLE;
    }

    mCurrentBufferSize = 0U;
    _mCurrentDeviceAllocSize = 0U;
    mDeviceSyncState = DEVICE_EMPTY;
}
