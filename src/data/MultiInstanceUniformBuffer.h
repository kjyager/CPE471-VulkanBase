#ifndef MULTI_INSTANCE_UNIFORM_BUFFER_H_
#define MULTI_INSTANCE_UNIFORM_BUFFER_H_
#include "UniformBuffer.h"
#include "vk_mem_alloc.h"
#include <exception>

// When enabled, automatic resizing doubles the size of the uniform buffer.
// When disabled, automatic reszing allocates the exact memory needed at that moment
#define MULTI_INSTANCE_UNIFORM_BUFFER_EXPONENTIAL_GROWTH

class UniformDataLayoutMismatchException : public std::exception
{
 public:
    UniformDataLayoutMismatchException(int64_t aExpectedBinding, int64_t aFoundBinding);
    UniformDataLayoutMismatchException(uint32_t aBinding, size_t aExpectedSize, size_t aActualSize);

    virtual const char* what() const noexcept override {return(_whatStr.c_str());}
 private:
    const std::string _whatStr;
};

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

    /// Returns number of instances contained in the buffer
    instance_index_t getInstanceCount() const {return(mInstanceCount);}
    /// Sets the number of instances contained in the buffer
    void setInstanceCount(instance_index_t aCount);
    /// Push back a new instance
    instance_index_t pushBackInstance();
    /// Push back a new instance and attach 'aDataInterface' to the instance. 
    /// Throws UniformDataLayoutMismatchException if 'aDataInterface' doesn't
    /// match internal layout set. 
    instance_index_t pushBackInstance(const UniformDataInterfaceSet& aDataInterface);

    /// Returns a set of raw data interfaces for the instance at 'aInstanceIndex' 
    /// 
    /// NOTE: Interface sets are created when first requested, then reused internally and by this function. 
    /// If getTypedInstanceDataInterfaces() has already been called on 'aInstanceIndex' this 
    /// function will contain UniformDataInterfacePtr instances pointing to the typed version, which
    /// may NOT be castable to UniformRawDataPtr. Use dynamic_cast to be safe. 
    UniformDataInterfaceSet getInstanceDataInterfaces(instance_index_t aInstanceIndex);

    /// TODO
    void setInstanceDataInterfaces(instance_index_t aInstanceIndex, const UniformDataInterfaceSet& aDataInterface);

    /// Free any data interface created for instance at 'aInstanceIndex'.
    /// 
    /// NOTE: The internal copy of the interface set is freed, but external
    /// references to this set will remain. However changes to those references
    /// will no longer effect this buffer. 
    void freeInstanceDataInterfaces(instance_index_t aInstanceIndex);

    /// Returns current instance capcity of the buffer. Will always be >= instance count
    instance_index_t getCapcity() const {return(mCapacity);}
    /// Set the instance capacity of the buffer. 'aCapacity' must be >= instance count.
    void setCapcity(instance_index_t aCapacity);
    /// Set capcity to exactly match number of instances. Best to call when no instances will be added or removed. 
    void resizeToFit();

    /// Returns number of UniformDataLayout objects attached for this buffer. 
    size_t boundLayoutCount() const {return(mBoundLayouts.size());}

    /// Returns the total size of an instance (excluding padding)
    size_t getInstanceDataSize() const {return(mBoundLayouts.getTotalPaddedSize(1));}

    /// Returns true if any bound uniform data is dirtied.
    bool isBoundDataDirty() const;
    /// Check if any of the bound uniform data has been dirtied and set deviceSyncState accordingly 
    void pollBoundData() const;

    DeviceSyncStateEnum getDeviceSyncState() const {pollBoundData(); return(mDeviceSyncState);}

    /// Update the device with the uniform buffer contents only if the data is out of sync with the device
    virtual void updateDevice() override;

    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    /// Get the offset of a particular binding point RELATIVE to the start of the instance block.
    size_t getBoundDataOffset(uint32_t aBindPoint) const;

    /// Get the offset of data bound at binding point 'aBindPoint' relative to instance 'aInstanceIndex'
    size_t getBoundDataOffset(uint32_t aBindPoint, instance_index_t aInstanceIndex) const;

    /// Get the map of descriptor set layout bindings. Each instance block shares this list. 
    const std::map<uint32_t, VkDescriptorSetLayoutBinding>& getDescriptorSetLayoutBindings() const;

    /** Returns handle for a descriptor set layout object matching the uniform buffer. 
      * This object is invalidated or deleted under the following circumstances:
      *   - freeAndReset() is called on UniformBuffer instance
      */
    VkDescriptorSetLayout getDescriptorSetLayout() const {return(mDescriptorSetLayout);}

    /// Get list of binding info for the bound layouts
    std::map<uint32_t, VkDescriptorBufferInfo> getDescriptorBufferInfos() const;

    virtual const VkBuffer& getBuffer() const override {return(mUniformBuffer);}
    virtual size_t getBufferSize() const override {return(mAllocInfo.size);}

    virtual void freeAndReset() override {_cleanup();}

#ifdef UNIFORM_BUFFER_UNIT_TESTING
 public:
#else
 protected:
#endif 

    MultiInstanceUniformBuffer() = default;
    void createDescriptorSetLayout();
    void createBuffer(size_t aNewSize);
    void autoGrowCapcity(instance_index_t aNewMinimumCapacity);
    void resizeBuffer(size_t aNewSize);

    void updateSingleBinding(
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