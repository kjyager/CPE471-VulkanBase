#include "MultiInstanceUniformBuffer.h"
#include "vkutils/VmaHost.h"
#include <algorithm>
#include <cstring>
#include <string>

using instance_index_t = MultiInstanceUniformBuffer::instance_index_t;

inline static size_t sNextPowerOf2(size_t aValue){
    size_t maxBit = static_cast<size_t>(1U) << (std::numeric_limits<size_t>::digits - 1U);

    if(aValue == 0) return(1);
    else if(aValue & maxBit) return(std::numeric_limits<size_t>::max());
    for(size_t i_ = 0; i_ < std::numeric_limits<size_t>::digits; ++i_){
        if(((maxBit >>= 1) & aValue) != 0) return(maxBit << 1);
    }
    return(1);
}

UniformDataLayoutMismatchException::UniformDataLayoutMismatchException(int64_t aExpectedBinding, int64_t aFoundBinding)
: _whatStr(aExpectedBinding >= 0 && aFoundBinding < 0 ? "Expected layout at binding point " + std::to_string(aExpectedBinding) + ", but found none." : "Unexpected layout at binding point " + std::to_string(aFoundBinding))
{}

UniformDataLayoutMismatchException::UniformDataLayoutMismatchException(uint32_t aBinding, size_t aExpectedSize, size_t aActualSize)
: _whatStr("Data layout at binding point " + std::to_string(aBinding) + "does not have expected size. Expected: " + std::to_string(aExpectedSize) + " Found: " + std::to_string(aActualSize))
{}

MultiInstanceUniformBuffer::MultiInstanceUniformBuffer(
    const VulkanDeviceBundle& aDeviceBundle, // Device to create uniform buffer on
    const UniformDataLayoutSet& aUniformDataLayouts, // Set of uniform data layouts describing the uniform buffer data
    instance_index_t aInstanceCount, // Initial number of instances for which to allocate space.
    instance_index_t aCapacityHint, // Optional: Suggested total capacity to allocate. Must be >= instance count. 0 is automatic and sets capcity equal to instance count 
    VkShaderStageFlags aShaderStages // Optional: Shader stage flags to enable for this uniform buffer. Defaults to vertex and fragment. 
) 
:   mCurrentDevice(aDeviceBundle),
    mInstanceCount(aInstanceCount),
    mCapacity(std::max(aInstanceCount,
    aCapacityHint)),
    mBoundLayouts(aUniformDataLayouts),
    mBufferAlignmentSize(aDeviceBundle.physicalDevice.mProperties.limits.minUniformBufferOffsetAlignment),
    mPaddedBlockSize(sLayoutSetAlignedSize(mBoundLayouts, mBufferAlignmentSize))
{
    // Will probably crash before hitting this check due to init of mBufferAlignmentSize
    if(!aDeviceBundle.isValid()){
        throw std::runtime_error("MultiInstanceUniformBuffer may not be constructed with an invalid or partially valid device bundle!");
    }

    for(const std::pair<uint32_t, UniformDataLayoutPtr>& entry : mBoundLayouts){
        uint32_t binding = entry.first;
        const UniformDataLayoutPtr layout = entry.second;

        mLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{
            /* binding = */ binding,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            /* descriptorCount = */ 1,
            /* stageFlags = */ aShaderStages,
            /* pImmutableSamplers = */ nullptr
        });
    }

    createBuffer(mPaddedBlockSize * mCapacity);
    createDescriptorSetLayout();
    updateOffsets();
    mDeviceSyncState = DEVICE_IN_SYNC;
}

void MultiInstanceUniformBuffer::setInstanceCount(instance_index_t aCount){
    if(aCount > mCapacity){
        autoGrowCapcity(aCount);
    }
    mInstanceCount = aCount;
    updateOffsets();
}

instance_index_t MultiInstanceUniformBuffer::pushBackInstance(){
    ++mInstanceCount;
    if(mInstanceCount > mCapacity){
        autoGrowCapcity(mInstanceCount);
    }
    mDeviceSyncState = DEVICE_OUT_OF_SYNC;
    updateOffsets();
    return(mInstanceCount-1);
}

instance_index_t MultiInstanceUniformBuffer::pushBackInstance(const UniformDataInterfaceSet& aDataInterfaces){
    instance_index_t index = pushBackInstance();
    assertLayoutMatches(aDataInterfaces);
    std::for_each(aDataInterfaces.begin(), aDataInterfaces.end(), [](const auto& entry){entry.second->flagAsDirty();}); // Artificially flag as dirty since the new slot doesn't have this data.
    mBoundDataInterfaces[index] = aDataInterfaces;
    return(index);
}

UniformDataInterfaceSet MultiInstanceUniformBuffer::getInstanceDataInterfaces(instance_index_t aInstanceIndex){
    assertInstanceInbounds(aInstanceIndex);
    const auto& finder = mBoundDataInterfaces.find(aInstanceIndex);
    if(finder != mBoundDataInterfaces.end()){
        return(finder->second);
    }else{
        UniformDataInterfaceSet& newSet = mBoundDataInterfaces[aInstanceIndex];
        for(const auto& layoutEntry : mBoundLayouts){
            newSet[layoutEntry.first] = std::static_pointer_cast<UniformDataInterface>(UniformRawData::create(layoutEntry.second->getDataSize()));
        }
        return(newSet);
    }
}

void MultiInstanceUniformBuffer::setInstanceDataInterfaces(instance_index_t aInstanceIndex, const UniformDataInterfaceSet& aDataInterfaces){
    assertInstanceInbounds(aInstanceIndex);
    assertLayoutMatches(aDataInterfaces);
    std::for_each(aDataInterfaces.begin(), aDataInterfaces.end(), [](const auto& entry){entry.second->flagAsDirty();}); // Artificially flag as dirty since the new slot doesn't have this data.
    mBoundDataInterfaces[aInstanceIndex] = aDataInterfaces;
    mDeviceSyncState = DEVICE_OUT_OF_SYNC;
}

void MultiInstanceUniformBuffer::freeInstanceDataInterfaces(instance_index_t aInstanceIndex){
    const auto& finder = mBoundDataInterfaces.find(aInstanceIndex);
    if(finder != mBoundDataInterfaces.end()) mBoundDataInterfaces.erase(finder);
}

void MultiInstanceUniformBuffer::setCapcity(instance_index_t aCapacity){
    if(aCapacity == mCapacity) return;
    mCapacity = aCapacity > mCapacity ? aCapacity : std::max(aCapacity, mInstanceCount);
    resizeBuffer(mCapacity * mPaddedBlockSize);
}

void MultiInstanceUniformBuffer::resizeToFit(){
    if(mCapacity != mInstanceCount){
        mCapacity = mInstanceCount;
        resizeBuffer(mCapacity * mPaddedBlockSize);
    }
}

bool MultiInstanceUniformBuffer::isBoundDataDirty() const{
    if(mDeviceSyncState != DEVICE_IN_SYNC) return true;

    for(const std::pair<instance_index_t, UniformDataInterfaceSet>& mapEntry : mBoundDataInterfaces){
        for(const std::pair<uint32_t, UniformDataInterfacePtr>& setEntry : mapEntry.second){
            if(setEntry.second->isDataDirty()) return true;
        }
    }
    return false;
}

void MultiInstanceUniformBuffer::pollBoundData() const{
    for(const std::pair<instance_index_t, UniformDataInterfaceSet>& mapEntry : mBoundDataInterfaces){
        for(const std::pair<uint32_t, UniformDataInterfacePtr>& setEntry : mapEntry.second){
            if(setEntry.second->isDataDirty()){
                mDeviceSyncState = DEVICE_OUT_OF_SYNC;
                return;
            }
        }
    }
}

void MultiInstanceUniformBuffer::updateDevice() {
    std::vector<UniformDataInterfacePtr> cleanedPtrs;
    cleanedPtrs.reserve(mBoundDataInterfaces.size() * mBoundLayouts.size());

    for(const std::pair<instance_index_t, UniformDataInterfaceSet>& mapEntry : mBoundDataInterfaces){
        for(const std::pair<uint32_t, UniformDataInterfacePtr>& setEntry : mapEntry.second){
            if(setEntry.second->isDataDirty()){
                updateSingleBinding(mapEntry.first, setEntry.first, setEntry.second);
                cleanedPtrs.emplace_back(setEntry.second);
            }
        }
    }
    std::for_each(cleanedPtrs.begin(), cleanedPtrs.end(), [](const UniformDataInterfacePtr& aPtr){aPtr->flagAsClean();});
    mDeviceSyncState = DEVICE_IN_SYNC;
}

size_t MultiInstanceUniformBuffer::getBoundDataOffset(uint32_t aBindPoint) const{
    return(mBoundLayouts.getBoundDataOffset(aBindPoint, mBufferAlignmentSize));
}

size_t MultiInstanceUniformBuffer::getBoundDataOffset(uint32_t aBindPoint, instance_index_t aInstanceIndex) const{
    assertInstanceInbounds(aInstanceIndex);
    return(mPaddedBlockSize*aInstanceIndex + getBoundDataOffset(aBindPoint));
}

const std::vector<VkDescriptorSetLayoutBinding>& MultiInstanceUniformBuffer::getDescriptorSetLayoutBindings() const{
    return(mLayoutBindings);
}

// TODO: Use flyweight or warn about cost of excessive use. 
std::map<uint32_t, VkDescriptorBufferInfo> MultiInstanceUniformBuffer::getDescriptorBufferInfos() const{
    std::map<uint32_t, VkDescriptorBufferInfo> infos;
    for(const std::pair<uint32_t, UniformDataLayoutPtr>& setEntry : mBoundLayouts){
        infos.emplace(setEntry.first, 
        VkDescriptorBufferInfo{
            mUniformBuffer,
            mBoundLayouts.getBoundDataOffset(setEntry.first, mBufferAlignmentSize),
            setEntry.second->getDataSize()
        });
    }
    return(infos);
}

void MultiInstanceUniformBuffer::createDescriptorSetLayout(){
    VkDescriptorSetLayoutCreateInfo createInfo = {};
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = mLayoutBindings.size();
        createInfo.pBindings = mLayoutBindings.data();
    }
    if(vkCreateDescriptorSetLayout(mCurrentDevice.device, &createInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create descriptor set layout for MultiInstanceUniformBuffer!");
    }
}

void MultiInstanceUniformBuffer::createBuffer(size_t aNewSize){
    if(aNewSize == 0){
        throw std::runtime_error("Required size of uniform buffer is zero, and buffer creation cannot take place.");
    }

    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    VkBufferCreateInfo bufferInfo;
    {
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = aNewSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0U;
        bufferInfo.pQueueFamilyIndices = nullptr;
    }

    VmaAllocationCreateInfo allocInfo = {};
    {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mUniformBuffer, &mBufferAllocation, &mAllocInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate host visible memory for MultiInstanceUniformBuffer!");
    }

    mDeviceSyncState = DEVICE_OUT_OF_SYNC;
}

void MultiInstanceUniformBuffer::autoGrowCapcity(instance_index_t aNewMinimumCapacity){
    #ifdef MULTI_INSTANCE_UNIFORM_BUFFER_EXPONENTIAL_GROWTH
        mCapacity = sNextPowerOf2(aNewMinimumCapacity);
    #else
        mCapacity = aNewMinimumCapacity;
    #endif
    resizeBuffer(mPaddedBlockSize * mCapacity);
}

void MultiInstanceUniformBuffer::resizeBuffer(size_t aNewSize){
    if(mUniformBuffer != VK_NULL_HANDLE){
        VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);
        vmaDestroyBuffer(allocator, mUniformBuffer, mBufferAllocation);
        mUniformBuffer = VK_NULL_HANDLE;
    }
    createBuffer(aNewSize);
    updateDevice();
}

void MultiInstanceUniformBuffer::updateOffsets(){
    mBlockOffsets.resize(mInstanceCount);
    for(size_t i = 0; i < mBlockOffsets.size(); ++i){
        mBlockOffsets[i] = mPaddedBlockSize*i;
    }
}

void MultiInstanceUniformBuffer::updateSingleBinding(instance_index_t aInstance, uint32_t aBinding, const UniformDataInterfacePtr aInterface){
    VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);

    size_t bufferOffset = mPaddedBlockSize * aInstance;
    size_t blockOffset = mBoundLayouts.getBoundDataOffset(aBinding, mBufferAlignmentSize);
    size_t offset = bufferOffset + blockOffset;

    void* rawptr = nullptr;
    VkResult mapResult = vmaMapMemory(allocator, mBufferAllocation, &rawptr);

    if(mapResult == VK_SUCCESS && rawptr != nullptr){
        uint8_t* dst = reinterpret_cast<uint8_t*>(rawptr) + offset;
        memcpy(dst, aInterface->getData(), aInterface->getDataSize());
        vmaUnmapMemory(allocator, mBufferAllocation);
        rawptr = nullptr;
    }else{
        throw std::runtime_error("MultiInstanceUniformBuffer: Mapping to uniform buffer failed!");
    }
}

void MultiInstanceUniformBuffer::setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) {
    // Unused
}

void MultiInstanceUniformBuffer::uploadToDevice(VulkanDeviceHandlePair aDevicePair) {
    updateDevice();
}

void MultiInstanceUniformBuffer::finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) {
    // Unused
}

void MultiInstanceUniformBuffer::assertInstanceInbounds(instance_index_t aIndex) const {
    if(aIndex >= mInstanceCount) throw InstanceBoundError(aIndex, mInstanceCount);
}

void MultiInstanceUniformBuffer::assertLayoutMatches(const UniformDataInterfaceSet& aDataInterfaces) const {
    for(const auto& layoutEntry : mBoundLayouts){
    const auto& finder = aDataInterfaces.find(layoutEntry.first);
    if(finder == aDataInterfaces.end()){
        throw UniformDataLayoutMismatchException(layoutEntry.first, -1);
    }
    if(finder->second->getDataSize() != layoutEntry.second->getDataSize()){
        throw UniformDataLayoutMismatchException(layoutEntry.first, finder->second->getDataSize(), layoutEntry.second->getDataSize());
    }
    if(aDataInterfaces.size() != mBoundLayouts.size()){
        for(const auto& interfaceEntry : aDataInterfaces){
            if(mBoundLayouts.find(interfaceEntry.first) == mBoundLayouts.end()){
                throw UniformDataLayoutMismatchException(-1, interfaceEntry.first);
            }
        }
    }
}
}

void MultiInstanceUniformBuffer::_cleanup(){
    if(mUniformBuffer != VK_NULL_HANDLE){
        VmaAllocator allocator = VmaHost::getAllocator(mCurrentDevice);
        vmaDestroyBuffer(allocator, mUniformBuffer, mBufferAllocation);
        mUniformBuffer = VK_NULL_HANDLE;
        mBufferAllocation = VK_NULL_HANDLE;
    }

    if(mDescriptorSetLayout != VK_NULL_HANDLE){
        vkDestroyDescriptorSetLayout(mCurrentDevice.device, mDescriptorSetLayout, nullptr);
        mDescriptorSetLayout = VK_NULL_HANDLE;
    }
}