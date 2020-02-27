#ifndef KJY_DUAL_TRANSFER_BACKED_BUFFER_H_
#define KJY_DUAL_TRANSFER_BACKED_BUFFER_H_
#include <vk_mem_alloc.h>
#include "SyncedBuffer.h"
#include "UploadTransferBackedBuffer.h"

/** Data agnostic vulkan buffer which sits in local memory and is accessed for input and output
 * through a host visible staging buffer and memory mapping */
class DualTransferBackedBuffer : public virtual UploadTransferBackedBuffer, virtual public DownloadTransferBackedBufferInterface
{
 public:

    using UploadTransferBackedBuffer::UploadTransferBackedBuffer;

    virtual void recordDownloadTransferCommand(const VkCommandBuffer& aCmdBuffer) override;

    virtual size_t copyDataFromStage(uint8_t* aDst, size_t aSizeLimit = std::numeric_limits<size_t>::max());
};

#endif