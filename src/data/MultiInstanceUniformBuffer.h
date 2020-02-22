#ifndef MULTI_INSTANCE_UNIFORM_BUFFER_H_
#define MULTI_INSTANCE_UNIFORM_BUFFER_H_
#include "UniformBuffer.h"
#include "vk_mem_alloc.h"

// When enabled, automatic resizing doubles the size of the uniform buffer.
// When disabled, automatic reszing allocates the exact memory needed at that moment
#define MULTI_INSTANCE_UNIFORM_BUFFER_EXPONENTIAL_GROWTH

/** A uniform buffer encapsulation class designed for use with dynamic offsets
 *  when drawing multiple times with the same uniform descriptor layout and differing data
 */
class MultiInstanceUniformBuffer : public DirectlySyncedBufferInterface
{
 public:

    /// (uint32_t) Index for an instance of uniform data within multi-instance uniform buffer
    using instance_index_t = uint32_t;

    explicit MultiInstanceUniformBuffer(
        const VulkanDeviceBundle& aDeviceBundle, // Device to create uniform buffer on
        const UniformDataLayoutSet& aUniformDataLayouts, // Set of uniform data layouts describing the uniform buffer data
        instance_index_t aInstanceCount, // Initial number of instances for which to allocate space.
        instance_index_t aCapacityHint = 0, // Optional: Suggested total capacity to allocate. Must be >= instance count. 0 is automatic and sets capcity equal to instance count 
        VkShaderStageFlags aShaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT // Optional: Shader stage flags to enable for this uniform buffer. Defaults to vertex and fragment. 
    );

    virtual instance_index_t getInstanceCount() const {return(mInstanceCount);}
    virtual void setInstanceCount(instance_index_t aCount);
    /// Push back a new instance, optionally initializing it with the raw data in 'aInitData'
    virtual instance_index_t pushBackInstance(const uint8_t* aInitData = nullptr);

    /// Returns current instance capcity of the buffer. Will always be >= instance count
    virtual instance_index_t getCapcity() const {return(mCapacity);}
    /// Set the instance capacity of the buffer. 'aCapacity' must be >= instance count.
    virtual void setCapcity(instance_index_t aCapacity);
    /// Set capcity to exactly match number of instances. Best to call when no instances will be added or removed. 
    virtual void resizeToFit();

    virtual size_t boundLayoutCount() const {return(mBoundLayouts.size());}

    /// Returns true if any bound uniform data is dirtied.
    virtual bool isBoundDataDirty() const;
    /// Check if any of the bound uniform data has been dirtied and set deviceSyncState accordingly 
    virtual void pollBoundData() const;

    virtual DeviceSyncStateEnum getDeviceSyncState() const {pollBoundData(); return(mDeviceSyncState);}

    /// Update the device with the uniform buffer contents only if the data is out of sync with the device
    virtual void updateDevice() override;

    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    /// Get the offset of a particular binding point RELATIVE to the start of the instance block.
    virtual size_t getBoundDataOffset(uint32_t aBindPoint) const;

    /// Get the offset of data bound at binding point 'aBindPoint' relative to instance 'aInstanceIndex'
    virtual size_t getBoundDataOffset(uint32_t aBindPoint, instance_index_t aInstanceIndex) const;

    /// Get the map of descriptor set layout bindings. Each instance block shares this list. 
    virtual const std::map<uint32_t, VkDescriptorSetLayoutBinding>& getDescriptorSetLayoutBindings() const;

    /** Returns handle for a descriptor set layout object matching the uniform buffer. 
      * This object is invalidated or deleted under the following circumstances:
      *   - freeAndReset() is called on UniformBuffer instance
      */
    virtual VkDescriptorSetLayout getDescriptorSetLayout() const {return(mDescriptorSetLayout);}

    /// Get list of binding info for the bound layouts
    virtual std::map<uint32_t, VkDescriptorBufferInfo> getDescriptorBufferInfos() const;

    virtual const VkBuffer& getBuffer() const override {return(mUniformBuffer);}
    virtual size_t getBufferSize() const override {return(mAllocInfo.size);}

    virtual void freeAndReset() override {_cleanup();}

 protected:
    MultiInstanceUniformBuffer() = default;
    virtual void createDescriptorSetLayout();
    virtual void createBuffer(size_t aNewSize);
    virtual void autoGrowCapcity(instance_index_t aNewMinimumCapacity);
    virtual void resizeBuffer(size_t aNewSize);

    virtual void updateSingleBinding(
        instance_index_t aInstance,
        uint32_t aBinding,
        const UniformDataInterfacePtr aInterface
    );

    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) override;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;

    VulkanDeviceHandlePair mCurrentDevice = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    instance_index_t mInstanceCount;
    instance_index_t mCapacity;

    // Store the set of layouts used in each instance block
    const UniformDataLayoutSet mBoundLayouts;

    // Maps binding point to descriptor set layout binding. 
    std::map<uint32_t, VkDescriptorSetLayoutBinding> mLayoutBindings; 

    // Map instance block indices to interfaces that can be used to modify uniform data in the block. 
    std::map<instance_index_t, UniformDataInterfaceSet> mBoundDataInterfaces;

    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

    mutable DeviceSyncStateEnum mDeviceSyncState = DEVICE_EMPTY;

    const VkDeviceSize mBufferAlignmentSize = 16U; 
    const size_t mPaddedBlockSize;

    VkBuffer mUniformBuffer = VK_NULL_HANDLE;
    VmaAllocation mBufferAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mAllocInfo;

 private:
    void _cleanup(); 
};

#endif