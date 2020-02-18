#include "../utils/common.h"
#include <array>
#include <vector>
#include <vulkan/vulkan.h>

template<
    typename VertexT, uint32_t binding, size_t attribute_count,
    uint32_t override_stride = 0u, VkVertexInputRate vertex_rate = VK_VERTEX_INPUT_RATE_VERTEX
>
class StaticVertexInputTemplate
{
 public:
    using vertex_t = VertexT; 

    StaticVertexInputTemplate() {}
    StaticVertexInputTemplate(const std::array<VkVertexInputAttributeDescription, attribute_count>& aInputAttributes) : mAttributes(aInputAttributes) {}

    /// Convenience, technically unecessary. 
    static uint32_t getBinding() {return(sInputBinding.binding);}
    static const VkVertexInputBindingDescription& getBindingDescription() {return(sInputBinding);}
    const std::array<VkVertexInputAttributeDescription, attribute_count>& getAttributeDescriptions() const {return(mAttributes);}

    const std::array<VkVertexInputAttributeDescription, attribute_count> mAttributes;

    static const size_t _sVertexSize = sizeof(vertex_t);
    static const VkVertexInputBindingDescription constexpr sInputBinding = {
        /* binding = */ binding,
        /* stride = */  (override_stride > 0) ? override_stride : sizeof(vertex_t),
        /* inputRate = */ vertex_rate
    };
};

template<typename VertexT>
class VertexInputTemplate
{
 public:
    using vertex_t = VertexT; 

    VertexInputTemplate(
        uint32_t aBinding, const std::initializer_list<VkVertexInputAttributeDescription>& aInputAttributes,
        uint32_t aStrideOverride = 0, VkVertexInputRate aInputRate = VK_VERTEX_INPUT_RATE_VERTEX
    );

    VertexInputTemplate(
        uint32_t aBinding, std::vector<VkVertexInputAttributeDescription>&& aInputAttributes,
        uint32_t aStrideOverride = 0, VkVertexInputRate aInputRate = VK_VERTEX_INPUT_RATE_VERTEX
    );

    /// Get and set the binding point of the input.
    /// This is the only get/set as any more significant changes to
    /// the vertex input justifies making a separate instance or class. 
    uint32_t getBinding() const {return(_mInputBinding.binding);}
    void setBinding(uint32_t aBinding) {_mInputBinding.binding = aBinding;}

    constexpr size_t getVertexSize() const noexcept { return(_sVertexSize); }

    const VkVertexInputBindingDescription& getBindingDescription() const {return(_mInputBinding);}
    const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const {return(_mAttributes); }

 private:

    static const size_t _sVertexSize = sizeof(vertex_t);
    VkVertexInputBindingDescription _mInputBinding;

    const std::vector<VkVertexInputAttributeDescription> _mAttributes;
};

template<typename VertexT>
VertexInputTemplate<VertexT>::VertexInputTemplate(
    uint32_t aBinding, const std::initializer_list<VkVertexInputAttributeDescription>& aInputAttributes,
    uint32_t aStrideOverride, VkVertexInputRate aInputRate
)
    : _mAttributes(aInputAttributes)
{
    _mInputBinding.binding = aBinding;
    _mInputBinding.stride = aStrideOverride > 0 ? aStrideOverride : sizeof(VertexT);
    _mInputBinding.inputRate = aInputRate;
}

template<typename VertexT>
VertexInputTemplate<VertexT>::VertexInputTemplate(
    uint32_t aBinding, std::vector<VkVertexInputAttributeDescription>&& aInputAttributes,
    uint32_t aStrideOverride, VkVertexInputRate aInputRate
)
    : _mAttributes(std::move(aInputAttributes))
{
    _mInputBinding.binding = aBinding;
    _mInputBinding.stride = aStrideOverride > 0 ? aStrideOverride : sizeof(VertexT);
    _mInputBinding.inputRate = aInputRate;
}