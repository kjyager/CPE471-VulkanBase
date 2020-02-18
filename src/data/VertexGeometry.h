#ifndef VERTEX_GEOMETRY_H_
#define VERTEX_GEOMETRY_H_

#include "utils/common.h"
#include "SyncedBuffer.h"
#include "UploadTransferBackedBuffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <cstring>

template<typename VertexType, typename IndexType = uint32_t>
class IndexedVertexGeometry : public virtual UploadTransferBackedBufferInterface
{
 public:
    using vertex_t = VertexType;
    using index_t = IndexType;

    IndexedVertexGeometry() = default;
    IndexedVertexGeometry(const VulkanDeviceBundle& aDeviceBundle);

    virtual void setDevice(const VulkanDeviceBundle& aDeviceBundle); 

    virtual void setVertices(const std::vector<VertexType>& aVertices);
    virtual void setIndices(const std::vector<index_t>& aIndices);

    virtual bool awaitingUploadTransfer() const {return(mVertexBuffer.awaitingUploadTransfer() || mIndexBuffer.awaitingUploadTransfer());}

    /// Records commands to upload both the index and attribute buffers to device local memory.
    /// Commands are recorded into aCmdBuffer. 
    virtual void recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) override;
    
    /// Returns size of staging buffer, which is the combined size of both the vertex and index buffer
    virtual size_t getBufferSize() const override;
    
    /// Same as calling getVertexBuffer()
    virtual const VkBuffer& getBuffer() const override {return(getVertexBuffer());}
    virtual const VkBuffer& getVertexBuffer() const {return(mVertexBuffer.getBuffer());}
    virtual const VkBuffer& getIndexBuffer() const {return(mIndexBuffer.getBuffer());}

    virtual void freeStagingBuffer();
    virtual void freeAndReset() override;

 protected:

    UploadTransferBackedBuffer mVertexBuffer; // buffer of vertex_t
    UploadTransferBackedBuffer mIndexBuffer;  // buffer of index_t

};

/// Triangle mesh geometry formed from a single set of vertex attributes, and one or more 
/// 'shapes' specified as lists of vertex indices. 
template<typename VertexType, typename IndexType = uint32_t>
class MultiShapeGeometry : public IndexedVertexGeometry<VertexType, IndexType>
{
 public:
    using vertex_t = VertexType;
    using index_t = IndexType;
    using IndexedVertexGeometry<VertexType, IndexType>::IndexedVertexGeometry;

    /// Return the number of shapes. 
    virtual size_t shapeCount() const {return(mShapeIndexBufferOffsets.size());}

    /// Add a new shape defined by drawing triangles indexed by 'aIndices'
    virtual void addShape(const std::vector<index_t>& aIndices) {
        mShapeIndexBufferOffsets.emplace_back(mIndicesConcat.size());
        mIndicesConcat.reserve(mIndicesConcat.size() + aIndices.size());
        mIndicesConcat.insert(mIndicesConcat.end(), aIndices.begin(), aIndices.end());
    }

    /// Same as calling addShape() with same arguments.
    virtual void setIndices(const std::vector<index_t>& aIndices) override {addShape(aIndices);};

 protected:
    std::vector<size_t> mShapeIndexBufferOffsets;
    std::vector<index_t> mIndicesConcat; 
};

/// 'Frozen' version of MultiShapeGeometry. Deletes methods which modify the geometry and would require
/// re-upload to the GPU. 
template<typename VertexType, typename IndexType = uint32_t>
class FrozenMultiShapeGeometry : public MultiShapeGeometry<VertexType, IndexType>
{
 public:
    using vertex_t = VertexType;
    using index_t = IndexType;
    using MultiShapeGeometry<VertexType, IndexType>::MultiShapeGeometry;

    /// DELETED! Cannot add new shapes to MultiShapeGeometry that has been frozen. 
    virtual void addShape(const std::vector<index_t>& aIndices) override = delete;
    /// DELETED! Cannot add new shapes to MultiShapeGeometry that has been frozen. 
    virtual void setIndices(const std::vector<index_t>& aIndices) override = delete;
    /// DELETED! Cannot change device on frozen MultiShapeGeometry
    virtual void setDevice(const VulkanDeviceBundle& aDeviceBundle) override = delete; 
    /// DELETED! Cannot change vertices of frozen MultiShapeGeometry
    virtual void setVertices(const std::vector<VertexType>& aVertices) override = delete;
    /// DELETED! Cannot initiate transfer command from frozen MultiShapeGeometry
    virtual void recordUploadTransferCommand(const VkCommandBuffer& aCmdBuffer) override = delete;

 protected:
    std::vector<size_t> mShapeIndexBufferOffsets;
    std::vector<index_t> mIndicesConcat; 
};

/* TODO: This class uses very naive memory allocation which will be brutally inefficient at scale. */
template<typename VertexType>
class HostVisVertexAttrBuffer : public DirectlySyncedBufferInterface
{
 public:
    using vertex_t = VertexType; 

    HostVisVertexAttrBuffer(){}
    explicit HostVisVertexAttrBuffer(const std::vector<VertexType>& aVertices, const VulkanDeviceBundle& aDeviceBundle = {}, bool aSkipDeviceUpload = false) : mCpuVertexData(aVertices) {
        if(aDeviceBundle.isValid() && !aSkipDeviceUpload) updateDevice(aDeviceBundle); 
    }

    // Disallow copy to avoid creating invalid instances. HostVisVertexAttrBuffer(s) should be combined with shared_ptrs or made intrusive. 
    HostVisVertexAttrBuffer(const HostVisVertexAttrBuffer& aOther) = delete; 

    virtual ~HostVisVertexAttrBuffer(){
        // Warning if cleanup wasn't explicit to teach responsibility
        if(mVertexBuffer != VK_NULL_HANDLE || mVertexBufferMemory != VK_NULL_HANDLE){
            std::cerr << "Warning! HostVisVertexAttrBuffer object destroyed before buffer was freed" << std::endl;
            _cleanup(); 
        }
    }

    virtual DeviceSyncStateEnum getDeviceSyncState() const override {return(mDeviceSyncState);}
    virtual void updateDevice() override;
    virtual void updateDevice(const VulkanDeviceBundle& aDevicePair);
    virtual VulkanDeviceHandlePair getCurrentDevice() const override {return(mCurrentDevice);}

    virtual size_t getBufferSize() const override {return(mCurrentBufferSize);}

    virtual const VkBuffer& getBuffer() const override {return(mVertexBuffer);}

    virtual void freeAndReset() override {_cleanup(); mCpuVertexData.clear();}

    /** Clears all vertex data held as member data on the class instance, but
     * leaves buffer data on device untouched. After flush, device is not
     * considered 'DEVICE_OUT_OF_SYNC', but instead marked 'CPU_DATA_FLUSHED'
     */
    virtual void flushCpuData() {
        mCpuVertexData.clear();
        mDeviceSyncState = mDeviceSyncState == DEVICE_IN_SYNC ? CPU_DATA_FLUSHED : mDeviceSyncState;
    }

    virtual size_t vertexCount() const {return(mCpuVertexData.size());}
    virtual std::vector<VertexType>& getVertices() {return(mCpuVertexData); mDeviceSyncState = DEVICE_OUT_OF_SYNC;}
    virtual const std::vector<VertexType>& getVertices() const {return(getVerticesConst());}
    virtual const std::vector<VertexType>& getVerticesConst() const {return(mCpuVertexData);}

    virtual void setVertices(const std::vector<VertexType>& aVertices) {mCpuVertexData = aVertices; mDeviceSyncState = DEVICE_OUT_OF_SYNC;}

 protected:

    virtual void setupDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;
    virtual void uploadToDevice(VulkanDeviceHandlePair aDevicePair) override;
    virtual void finalizeDeviceUpload(VulkanDeviceHandlePair aDevicePair) override;


    std::vector<VertexType> mCpuVertexData;
    DeviceSyncStateEnum mDeviceSyncState = DEVICE_EMPTY;
    

    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize mCurrentBufferSize = 0U;
    VulkanDeviceHandlePair mCurrentDevice = {VK_NULL_HANDLE, VK_NULL_HANDLE};

 private:
    void _cleanup();

    VkDeviceSize _mCurrentDeviceAllocSize = 0U; 
};

#include "VertexGeometry.inl"

#endif