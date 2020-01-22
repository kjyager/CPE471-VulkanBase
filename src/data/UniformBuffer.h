#ifndef UNIFORM_BUFFER_H_
#define UNIFORM_BUFFER_H_

// #define TRANSFER_BY_MEMCPY

#include "../utils/common.h"
#include "DeviceSyncedBuffer.h"
#include <vulkan/vulkan.h>
#include <memory>

/* Interface class for abstracting away multiple duplicate uniform buffers */
class UniformHandler
{
 protected:
    // Friending like this is contrived and kinda hacky, but I want to prevent students from calling these methods.
    friend class VulkanSetupBaseApp; 

    virtual void init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags) = 0;
    virtual void free() = 0;
    virtual void reinit() = 0;

    virtual VkBuffer getBufferHandle(size_t aIndex) const = 0;
    virtual size_t getBufferCount() const = 0;
    virtual void prepareBuffer(size_t aIndex) = 0;
};

using UniformHandlerPtr = std::shared_ptr<UniformHandler>; 

template<typename UniformStruct>
class UniformBuffer : public DeviceSyncedBuffer
{
 public:
    using uniform_struct_t = UniformStruct; 

    explicit UniformBuffer(uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    /// Get and set the binding point of the buffer.
    /// This is the only get/set as any more significant changes to
    /// the layout justifies making a separate instance or class. 
    uint32_t getBinding() const {return(mDescriptorSetLayoutBinding.binding);}
    void setBinding(uint32_t aBinding) {mDescriptorSetLayoutBinding.binding = aBinding;}

    virtual UniformStruct& getUniformData() {mDeviceSyncState = DEVICE_OUT_OF_SYNC; return(mCpuStruct);}
    virtual const UniformStruct& getUniformData() const {return(mCpuStruct);}
    virtual const UniformStruct& getUniformDataConst() const {return(mCpuStruct);}

    virtual void setUniformData(const UniformStruct& aUniformStruct) {mCpuStruct = aUniformStruct; mDeviceSyncState = DEVICE_OUT_OF_SYNC;}

    constexpr size_t getUniformStructSize() const noexcept {return(smUniformStructSize);}

    const VkDescriptorSetLayoutBinding& getDescriptorSetLayoutBinding() const {return(mDescriptorSetLayoutBinding);}
    const VkDescriptorSetLayout& getDescriptorSetLayout() const {return(mDescriptorSetLayout);} 

    virtual DeviceSyncStateEnum getDeviceSyncState() const override {return(mDeviceSyncState);}
    virtual void updateDevice(VulkanDeviceHandlePair aDevicePair = {}) override;
    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    virtual const VkBuffer& getBuffer() const override {return(mUniformBuffer);}

    virtual void freeBuffer() override {_cleanup();}

 protected:
    void createDescriptorSetLayout();
    void createUniformBuffer();
    
    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) override;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;

    static const size_t smUniformStructSize = sizeof(uniform_struct_t); 

    VkDescriptorSetLayoutBinding mDescriptorSetLayoutBinding;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

    uniform_struct_t mCpuStruct; 
    DeviceSyncStateEnum mDeviceSyncState = DEVICE_EMPTY;

    VkBuffer mUniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mUniformBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize mCurrentBufferSize = 0U;
    VulkanDeviceHandlePair mCurrentDevice = {VK_NULL_HANDLE, VK_NULL_HANDLE};

 private:
    void _cleanup(); 

    VkDeviceSize _mCurrentDeviceAllocSize = 0U; 

};

template<typename UniformStruct>
class UniformStructBufferHandler : protected UniformHandler
{
 public:
    using buffer_t = UniformBuffer<UniformStruct>;
    using uniform_struct_t = UniformStruct;

    UniformStructBufferHandler(){}

    virtual void pushUniformStruct(const uniform_struct_t& aStruct);

 protected:
    virtual void init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags) override;
    virtual void free() override;
    virtual void reinit() override;

    virtual VkBuffer getBufferHandle(size_t aIndex) const override {return(aIndex < mBuffers.size() ? mBuffers.at(aIndex) : VK_NULL_HANDLE);}
    virtual size_t getBufferCount() const override {return(mBuffers.size());}
    virtual void prepareBuffer(size_t aIndex) override;

    uniform_struct_t mStagedData;
    std::vector<buffer_t> mBuffers;
    std::vector<bool> mIsUpdated; 
};

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags){
    assert(mBuffers.size() == 0);
    assert(mIsUpdated.size() == 0);
    assert(aBufferCount > 0);
    for(size_t i = 0; i < aBufferCount; ++i){
        mBuffers.emplace_back(aBinding, aDevicePair, aStageFlags);
        assert(mBuffers[i].getDeviceSyncState() == DEVICE_IN_SYNC);
        mIsUpdated.emplace_back(true);
    }
}

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::free(){
    for(buffer_t& buffer : mBuffers){
        buffer.freeBuffer();
    }
    mBuffers.clear();
    mIsUpdated.clear();
}

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::reinit(){
    free();
    init();
}

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::pushUniformStruct(const uniform_struct_t& aStruct){
    mStagedData = aStruct;
    mIsUpdated.assign(mIsUpdated.size(), false);
}

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::prepareBuffer(size_t aIndex){
    assert(aIndex < mBuffers.size());
    if(!mIsUpdated[aIndex]){
        mBuffers[aIndex].setUniformData(mStagedData); 
        mBuffers[aIndex].updateDevice(); 
        mIsUpdated[aIndex] = true;
    }
}

template<typename UniformStruct>
UniformBuffer<UniformStruct>::UniformBuffer(uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags)
: mCurrentDevice(aDevicePair) {
    mDescriptorSetLayoutBinding.binding = aBinding;
    mDescriptorSetLayoutBinding.stageFlags = aStageFlags;
    mDescriptorSetLayoutBinding.descriptorCount = 1;
    mDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    updateDevice();
}

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::createDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = 1;
        createInfo.pBindings = &mDescriptorSetLayoutBinding; 
    }

    assert(!mCurrentDevice.isNull());

    if(vkCreateDescriptorSetLayout(mCurrentDevice.device, &createInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create descriptor set layout for uniform buffer!");
    }
}

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::createUniformBuffer(){
    if(mDeviceSyncState == DEVICE_EMPTY || smUniformStructSize != mCurrentBufferSize){
        VkBufferCreateInfo createInfo;
        {
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.size = smUniformStructSize;
            createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0U;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        if(vkCreateBuffer(mCurrentDevice.device, &createInfo, nullptr, &mUniformBuffer) != VK_SUCCESS){
            throw std::runtime_error("Failed to create uniform buffer!"); 
        }
    }
}

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::updateDevice(VulkanDeviceHandlePair aDevicePair){
    if(!aDevicePair.isNull() && aDevicePair != mCurrentDevice){
        _cleanup();
        mCurrentDevice = aDevicePair; 
        createDescriptorSetLayout();
    }

    if(mCurrentDevice.isNull()){
        throw std::runtime_error("Attempting to updateDevice() from uniform buffer with no associated device!");
    }

    if(mDeviceSyncState == DEVICE_OUT_OF_SYNC || mDeviceSyncState == DEVICE_EMPTY){
        setupDeviceUpload(mCurrentDevice);
        uploadToDevice(mCurrentDevice);
        finalizeDeviceUpload(mCurrentDevice);
    }
}

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::setupDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    if(mUniformBuffer == VK_NULL_HANDLE)
        createUniformBuffer();
}

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::uploadToDevice(VulkanDeviceHandlePair aDevicePair){

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
        mCurrentBufferSize = smUniformStructSize;
    }

    void* mappedPtr = nullptr;
    VkResult mapResult = vkMapMemory(aDevicePair.device, mUniformBufferMemory, 0, _mCurrentDeviceAllocSize , 0, &mappedPtr);
    if(mapResult != VK_SUCCESS || mappedPtr == nullptr) throw std::runtime_error("Failed to map memory during uniform buffer upload!");
    {
        #ifdef TRANSFER_BY_MEMCPY
            memcpy(mappedPtr, &mCpuStruct, smUniformStructSize);
        #else
            // May induce side-effects, but cooler :) 
            UniformStruct* gpuStruct = reinterpret_cast<UniformStruct*>(mappedPtr);
            *gpuStruct = mCpuStruct;
        #endif

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

template<typename UniformStruct>
void UniformBuffer<UniformStruct>::finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair){
    mDeviceSyncState = DEVICE_IN_SYNC;
}
template<typename UniformStruct>
void UniformBuffer<UniformStruct>::_cleanup(){
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
    mDeviceSyncState = DEVICE_EMPTY;
}


#endif