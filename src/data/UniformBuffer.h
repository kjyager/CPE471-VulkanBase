#ifndef UNIFORM_BUFFER_H_
#define UNIFORM_BUFFER_H_

#include "../utils/common.h"
#include "DeviceSyncedBuffer.h"
#include <vulkan/vulkan.h>
#include <map>
#include <memory>

class UniformDataInterface
{
 public:
    virtual size_t getDataSize() const = 0;
    virtual size_t getPaddedDataSize() const = 0;
    virtual const uint8_t* getData() const = 0;

    virtual bool isDataDirty() const = 0;

 protected:
    friend class UniformBuffer;
    virtual void flagAsClean() = 0;

};

using UniformDataInterfacePtr = std::shared_ptr<UniformDataInterface>;

template<typename UniformStruct>
class UniformStructData : public UniformDataInterface
{
 public:
    using uniform_struct_t = UniformStruct;
    using ptr_t = std::shared_ptr<UniformStructData<uniform_struct_t>>;

    /** Create a new UniformStructData object with uninitalized data> */
    static ptr_t create() {return(ptr_t(new UniformStructData<uniform_struct_t>()));}

    virtual void pushUniformData(const UniformStruct& aStruct) {setStruct(aStruct);}

    virtual UniformStruct& getStruct() {mIsDirty = true; return(mCpuStruct);}
    virtual const UniformStruct& getStruct() const {return(mCpuStruct);}
    virtual const UniformStruct& getStructConst() const {return(mCpuStruct);}
    virtual void setStruct(const UniformStruct& aStruct) {mIsDirty = true; mCpuStruct = aStruct;}

 protected:
    UniformStructData(){}

    virtual size_t getDataSize() const override {return(_mDataSize);}
    virtual size_t getPaddedDataSize() const override {return(_mPaddedDataSize);}
    virtual const uint8_t* getData() const override {return(reinterpret_cast<const uint8_t*>(&mCpuStruct));}
    virtual bool isDataDirty() const override {return(mIsDirty);}
    virtual void flagAsClean() override {mIsDirty = false;}

    bool mIsDirty = false;
    uniform_struct_t mCpuStruct;

 private:
    const static size_t _mDataSize = sizeof(uniform_struct_t);
    const static size_t _mPaddedDataSize = sizeof(uniform_struct_t) + sizeof(uniform_struct_t) % 16; 
};

class UniformBuffer : public DeviceSyncedBuffer
{
 public:
    UniformBuffer(){}
    explicit UniformBuffer(const VulkanDeviceHandlePair& aDevicePair);

    virtual void bindUniformData(uint32_t aBindPoint, UniformDataInterfacePtr aUniformData, VkShaderStageFlags aStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    virtual size_t getBoundDataCount() const {return(mBoundUniformData.size());}

    /** Returns true if any bound uniform data is dirtied.*/
    virtual bool isBoundDataDirty() const;
    /** Check if any of the bound uniform data has been dirtied and set deviceSyncState accordingly */
    virtual void pollBoundData();

    virtual DeviceSyncStateEnum getDeviceSyncState() const override;
    virtual void updateDevice(VulkanDeviceHandlePair aDevicePair = {}) override;
    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    virtual size_t getBoundDataOffset(uint32_t aBindPoint) const;

    virtual VkDescriptorSetLayout getDescriptorSetLayout() const {return(mDescriptorSetLayout);}
    virtual std::vector<VkDescriptorBufferInfo> getDescriptorBufferInfos() const;
    virtual std::vector<uint32_t> getBoundPoints() const; 

    virtual const VkBuffer& getBuffer() const override {return(mUniformBuffer);}

    virtual void freeBuffer() override {_cleanup();}

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

 private:
    void _cleanup(); 

    VkDeviceSize _mCurrentDeviceAllocSize = 0U; 
};

#endif