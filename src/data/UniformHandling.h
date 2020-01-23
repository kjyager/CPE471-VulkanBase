#ifndef UNIFORM_HANDLING_H_
#define UNIFORM_HANDLING_H_
#include "UniformBuffer.h"
#include <memory>
#include <map>

/** Interface class for abstracting away multiple duplicate uniform buffers */
class UniformHandler
{
 protected:
    friend class UniformHandlerCollection;
    // Friending the app is contrived and kinda hacky, but I want to prevent students from calling these methods.
    friend class VulkanGraphicsApp; 

    virtual void init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags) = 0;
    virtual void free() = 0;
    virtual bool isInited() const = 0;

    virtual void prepareBuffer(size_t aIndex) = 0;

    virtual size_t getBufferCount() const = 0;

    virtual VkBuffer getBufferHandle(size_t aIndex) const = 0;
    virtual uint32_t getRepresentativeBinding() const = 0;
    virtual const VkDescriptorSetLayout& getRepresentativeDescriptorSetLayout() const = 0;
    virtual VkDeviceSize getRepresentativeOffset() const = 0;
    virtual VkDeviceSize getRepresentativeRange() const = 0;

};

// Alias UniformHandlerPtr to a smart pointer of UniformHandler 
using UniformHandlerPtr = std::shared_ptr<UniformHandler>; 


/** Implementation of UniformHandler that maintains multiple instances of one struct based uniform buffer. */
template<typename UniformStruct>
class UniformStructBufferHandler : public UniformHandler
{
 public:
    using buffer_t = UniformBuffer<UniformStruct>;
    using uniform_struct_t = UniformStruct;

    UniformStructBufferHandler(){}

    virtual void pushUniformStruct(const uniform_struct_t& aStruct);

 protected:
    virtual void init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags) override;
    virtual void free() override;
    virtual bool isInited() const override {return(mBuffers.size() > 0 );}

    virtual void prepareBuffer(size_t aIndex) override;

    virtual size_t getBufferCount() const override {return(mBuffers.size());}

    virtual VkBuffer getBufferHandle(size_t aIndex) const override {
        return(aIndex < mBuffers.size() ? mBuffers.at(aIndex).getBuffer() : VK_NULL_HANDLE);
    }
    virtual uint32_t getRepresentativeBinding() const override {
        assert(isInited() && mBuffers.size() > 0);
        return(mBuffers[0].getBinding());
    }
    virtual const VkDescriptorSetLayout& getRepresentativeDescriptorSetLayout() const override {
        assert(isInited() && mBuffers.size() > 0);
        return(mBuffers[0].getDescriptorSetLayout());
    }
    virtual VkDeviceSize getRepresentativeOffset() const override {
        return 0;
    }
    virtual VkDeviceSize getRepresentativeRange() const override {
        assert(isInited() && mBuffers.size() > 0);
        return(mBuffers[0].getUniformStructSize());
    }


    uniform_struct_t mStagedData;
    std::vector<buffer_t> mBuffers;
    std::vector<bool> mIsUpdated; 
};

/** An ordered associative collection mapping binding points (uint32_t) to uniform handlers (UniformHandlerPtr).
 * 
 * UniformHandlerCollection is an extension of std::map<uint32_t, UniformHandlerPtr> and possesses
 * the same functionality as std::map along with the added member functions. 
 */
class UniformHandlerCollection : public std::map<uint32_t, UniformHandlerPtr>
{
 public:
    /** std::map type this collection derives from */
    using derived_map_t = std::map<uint32_t, UniformHandlerPtr>;

    /** Free all contained buffers and clear the map */
    virtual void freeAllAndClear(); 
    void clear() {freeAllAndClear();}

    /** Returns the sum of all contained UniformHandlerPtr's getBufferCount() call. */ 
    virtual uint32_t getTotalUniformBufferCount() const;

    /** Return a vector of all descriptor layouts in the entire collection ordered first by buffer index then by binding point
     * The returned vector's size will be the same as 'getTotalUniformBufferCount()'
    */
    virtual std::vector<VkDescriptorSetLayout> unrollDescriptorLayouts() const;

    /** Return a vector of all buffer info for the entire collection ordered first by buffer index then by binding point
     * The returned vector's size will be the same as 'getTotalUniformBufferCount()'
    */
    virtual std::vector<VkDescriptorBufferInfo> unrollDescriptorBufferInfo() const;

    struct ExtraInfo{
        // All we need for now 
        uint32_t binding = 0;
    };

    /** Return a vector of 'UniformHandlerCollection::ExtraInfo' structs representing all buffers in the collection ordered first by buffer index then by binding point
     * buffers in the collection. The returned vector's size will be the same as 'getTotalUniformBufferCount()'
    */
    virtual std::vector<ExtraInfo> unrollExtraInfo() const;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////// Member Function Definitions for UniformStructBufferHandler ////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename UniformStruct>
void UniformStructBufferHandler<UniformStruct>::init(size_t aBufferCount, uint32_t aBinding, const VulkanDeviceHandlePair& aDevicePair, VkShaderStageFlags aStageFlags){
    assert(mBuffers.empty());
    assert(mIsUpdated.empty());
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

#endif