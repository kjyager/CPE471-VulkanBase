#ifndef UNIFORM_BUFFER_H_
#define UNIFORM_BUFFER_H_

#include "../utils/common.h"
#include "SyncedBuffer.h"
#include <vulkan/vulkan.h>
#include <map>
#include <memory>

inline static constexpr size_t sAlignData(size_t aDataSize, size_t aAlignSize){
    return ((aDataSize / aAlignSize + size_t(aDataSize % aAlignSize != 0)) * aAlignSize);
}

/** Pure virtual base class for detailing the layout of uniform data. Meant for 
 *enabling polymorphic to expose any typed C++ object or objects for use in uniform buffers
 */
class UniformDataLayout
{
 public:
    virtual ~UniformDataLayout() = default;
    virtual size_t getDataSize() const = 0;
    virtual size_t getPaddedDataSize(size_t aDeviceAlignmentSize) const = 0;
};

/** Pure virtual base class providing generic access to data kept in uniform buffers.
 *  Implementing classes are expected to report when their data has changed and should
 *  be synchronized with the GPU
 */
class UniformDataInterface : public virtual UniformDataLayout
{
 public:

    virtual const uint8_t* getData() const = 0;
    virtual bool isDataDirty() const = 0;

 protected:
    friend class UniformBuffer;
    friend class MultiInstanceUniformBuffer;

    virtual void flagAsClean() const = 0;
    virtual void flagAsDirty() const = 0;
};

using UniformDataLayoutPtr = std::shared_ptr<UniformDataLayout>;
using UniformDataInterfacePtr = std::shared_ptr<UniformDataInterface>;

/// Collection mapping binding points to uniform data
struct UniformDataLayoutSet : public std::map<uint32_t, UniformDataLayoutPtr>{
    using std::map<uint32_t, UniformDataLayoutPtr>::map;

    size_t getBoundDataOffset(uint32_t aBindPoint, size_t aAlignSize) const;
    size_t getTotalPaddedSize(size_t aAlignSize) const;
};

using UniformDataInterfaceSet = std::map<uint32_t, UniformDataInterfacePtr>;

/// Get the aligned size of an entire set of uniform data layouts
inline static size_t sLayoutSetAlignedSize(UniformDataLayoutSet aLayoutSet, size_t aAlignSize){
    size_t paddedSizeSum = 0;
    for(const std::pair<uint32_t, UniformDataLayoutPtr>& setEntry : aLayoutSet){
        paddedSizeSum = setEntry.second->getPaddedDataSize(aAlignSize);
    }
    return(sAlignData(paddedSizeSum, aAlignSize));
}

template<typename UniformStruct, size_t T_alignment_size = 16U>
class UniformStructDataLayout : virtual public UniformDataLayout
{
 public:
    using uniform_struct_t = UniformStruct;
    using ptr_t = std::shared_ptr<UniformStructDataLayout<uniform_struct_t>>;

    /** Create a new UniformStructData object with uninitalized data> */
    static ptr_t create() {return(ptr_t(new UniformStructDataLayout<uniform_struct_t>()));}

    virtual size_t getDataSize() const override {return(_mDataSize);}
    virtual size_t getDefaultPaddedDataSize() const {return(_mPaddedDataSize);}
    virtual size_t getDefaultAlignmentSize() const {return(T_alignment_size);}
    virtual size_t getPaddedDataSize(size_t aDeviceAlignmentSize) const override {return(sAlignData(_mPaddedDataSize, aDeviceAlignmentSize));}

 protected:

    const static size_t _mDataSize = sizeof(uniform_struct_t);
    const static size_t _mPaddedDataSize = sAlignData(_mDataSize, T_alignment_size); 
};


template<typename UniformStruct, size_t T_alignment_size = 16U>
class UniformStructData : virtual public UniformDataInterface, virtual public UniformStructDataLayout<UniformStruct, T_alignment_size>
{
 public:
    using uniform_struct_t = UniformStruct;
    using ptr_t = std::shared_ptr<UniformStructData<uniform_struct_t>>;

    UniformStructData() = default;

    /** Create a new UniformStructData object with uninitalized data> */
    static ptr_t create() {return(ptr_t(new UniformStructData<uniform_struct_t>()));}

    virtual void pushUniformData(const UniformStruct& aStruct) {setStruct(aStruct);}

    virtual UniformStruct& getStruct() {mIsDirty = true; return(mCpuStruct);}
    virtual const UniformStruct& getStruct() const {return(mCpuStruct);}
    virtual const UniformStruct& getStructConst() const {return(mCpuStruct);}
    virtual void setStruct(const UniformStruct& aStruct) {mIsDirty = true; mCpuStruct = aStruct;}

    virtual const uint8_t* getData() const override {return(reinterpret_cast<const uint8_t*>(&mCpuStruct));}
    virtual bool isDataDirty() const override {return(mIsDirty);}
    virtual void flagAsClean() const override {mIsDirty = false;}
    virtual void flagAsDirty() const override {mIsDirty = true;}

 protected:

    mutable bool mIsDirty = false;
    uniform_struct_t mCpuStruct;
};

/// Data is sized at runtime and must be heap allocated
class _UniformRawData_RT;

/// Data is sized at compile time and will be allocated as part of struct. 
template<size_t T_dataSize>
class _UniformRawData_CT;

class UniformRawData;
using UniformRawDataPtr = std::shared_ptr<UniformRawData>;

class UniformRawData : virtual public UniformDataLayout, virtual public UniformDataInterface
{
public:
    template<size_t T_dataSize>
    static UniformRawDataPtr create(const uint8_t* aData = nullptr){return(std::make_shared<_UniformRawData_CT<T_dataSize>>(aData));}
    static UniformRawDataPtr create(size_t aDataSize, const uint8_t* aData = nullptr){return(std::static_pointer_cast<UniformRawData>(std::make_shared<_UniformRawData_RT>(aDataSize, aData)));}

    virtual ~UniformRawData() = default;
    virtual uint8_t* getData() = 0;
    virtual size_t getDataSize() const {return(mSize);}
    virtual size_t getPaddedDataSize(size_t aDeviceAlignmentSize) const {return(sAlignData(mSize, aDeviceAlignmentSize));}

    virtual bool isDataDirty() const {return(mIsDirty);}
protected:
    UniformRawData() = default;
    UniformRawData(size_t aSize) : mSize(aSize){}
    virtual void flagAsClean() const override {mIsDirty = false;}
    virtual void flagAsDirty() const override {mIsDirty = true;}

    mutable bool mIsDirty = false;
    const size_t mSize = 0;
};

/// Data is sized at runtime and must be heap allocated
class _UniformRawData_RT : public UniformRawData
{
 public:
    _UniformRawData_RT() = default;
    _UniformRawData_RT(size_t aSize, const uint8_t* aData) : UniformRawData(aSize), mData(aSize) {
        if(aData != nullptr){
            mIsDirty = true;
            mData.assign(aData, aData + mSize);
        }
    }
    virtual ~_UniformRawData_RT() = default;

    virtual uint8_t* getData() {mIsDirty = true; return(mData.data());} 
    virtual const uint8_t* getData() const {return(mData.data());}
 protected:
    std::vector<uint8_t> mData;
};
/// Data is sized at compile time and will be allocated as part of struct. 
template<size_t T_dataSize>
class _UniformRawData_CT : public UniformRawData
{
 public:
    _UniformRawData_CT(const uint8_t* aData) : UniformRawData(T_dataSize){
        if(aData != nullptr){
            mIsDirty = true;
            for(size_t i = 0; i < T_dataSize; ++i){mData[i] = aData[i];}
        }
    }
    virtual ~_UniformRawData_CT() = default;

    virtual uint8_t* getData() {mIsDirty = true; return(mData.data());}
    virtual const uint8_t* getData() const {return(mData.data());}
 protected:
    std::array<uint8_t, T_dataSize> mData;
};

class UniformBuffer : public DirectlySyncedBufferInterface
{
 public:
    UniformBuffer(){}
    explicit UniformBuffer(const VulkanDeviceBundle& aDeviceBundle);

    virtual void bindUniformData(uint32_t aBindPoint, UniformDataInterfacePtr aUniformData, VkShaderStageFlags aStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    virtual size_t boundInterfaceCount() const {return(mBoundUniformData.size());}

    /** Returns true if any bound uniform data is dirtied.*/
    virtual bool isBoundDataDirty() const;
    /** Check if any of the bound uniform data has been dirtied and set deviceSyncState accordingly */
    virtual void pollBoundData();

    virtual DeviceSyncStateEnum getDeviceSyncState() const override;
    virtual void updateDevice() override;
    virtual void updateDevice(const VulkanDeviceBundle& aDevicePair);
    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    virtual size_t getBoundDataOffset(uint32_t aBindPoint) const;

    virtual std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayoutBindings() const;

    /** Returns handle for a descriptor set layout object matching the uniform buffer. 
      * This object is invalidated or deleted under the following circumstances:
      *   - freeAndReset() is called on UniformBuffer instance
      *   - updateDevice() is called with a new device
      *   - uniform data is bound or unbound from UniformBuffer instance
      */
    virtual VkDescriptorSetLayout getDescriptorSetLayout();
    /** Const version of getDescriptorSetLayout(). May return `VK_NULL_HANDLE` if
     *  non-const version of this call has not previously been called.
     */
    virtual VkDescriptorSetLayout getDescriptorSetLayout() const {return(mDescriptorSetLayout);}

    virtual std::vector<VkDescriptorBufferInfo> getDescriptorBufferInfos() const;
    virtual std::vector<uint32_t> getBoundPoints() const; 

    virtual const VkBuffer& getBuffer() const override {return(mUniformBuffer);}
    virtual size_t getBufferSize() const override {return(mCurrentBufferSize);}

    virtual void freeAndReset() override {_cleanup();}

 protected:
    virtual void createDescriptorSetLayout();
    virtual void createUniformBuffer();

    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) override;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;

    struct BoundUniformData{
        UniformDataInterfacePtr mDataInterface = nullptr;
        VkDescriptorSetLayoutBinding mLayoutBinding;
    };

    std::map<uint32_t, BoundUniformData> mBoundUniformData;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

    DeviceSyncStateEnum mDeviceSyncState = DEVICE_EMPTY;
    bool mLayoutOutOfDate = true;

    VkBuffer mUniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mUniformBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize mCurrentBufferSize = 0U;
    VulkanDeviceHandlePair mCurrentDevice = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceSize mBufferAlignmentSize = 16U; 

 private:
    void _cleanup(); 

    VkDeviceSize _mCurrentDeviceAllocSize = 0U; 
};

#endif
