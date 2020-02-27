#include "vkutils.h"
#include "VmaHost.h"
#include <cassert>
#include <array>

namespace vkutils
{

VulkanBasicRasterPipelineBuilder::VulkanBasicRasterPipelineBuilder(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle)
:   _mConstructionSet(aDevicePair, aChainBundle)
{
    VulkanRenderPipeline::_mLogicalDevice = aDevicePair.device;
}

void VulkanRenderPipeline::destroy(){
    vkDestroyPipeline(_mLogicalDevice, mGraphicsPipeline, nullptr);
    mGraphicsPipeline = VK_NULL_HANDLE;
    vkDestroyRenderPass(_mLogicalDevice, mRenderPass, nullptr);
    mRenderPass = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(_mLogicalDevice, mGraphicsPipeLayout, nullptr);
    mGraphicsPipeLayout = VK_NULL_HANDLE;
}

GraphicsPipelineConstructionSet& VulkanBasicRasterPipelineBuilder::setupConstructionSet(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle){
    _mLogicalDevice = aDevicePair.device;
    _mConstructionSet = GraphicsPipelineConstructionSet(aDevicePair, aChainBundle);
    return(_mConstructionSet);
}

void VulkanBasicRasterPipelineBuilder::build(const GraphicsPipelineConstructionSet& aFinalCtorSet){
    if(_mLogicalDevice != aFinalCtorSet.mDevicePair.device){
        throw std::runtime_error("Logical device assigned to VulkanBasicRasterPipelineBuilder does not match the device in the constructions set.");
    }
    _mConstructionSet = aFinalCtorSet;
    
    // Create pipeline layout object
    vkCreatePipelineLayout(aFinalCtorSet.mDevicePair.device, &aFinalCtorSet.mPipelineLayoutInfo, nullptr, &mGraphicsPipeLayout);

    VkPipelineDynamicStateCreateInfo dynamicStateInfo;{
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pNext = nullptr;
        dynamicStateInfo.flags = 0;
        dynamicStateInfo.dynamicStateCount = aFinalCtorSet.mDynamicStates.size();
        dynamicStateInfo.pDynamicStates = aFinalCtorSet.mDynamicStates.data();
    }

    std::array<VkAttachmentDescription, 2> standardAttachments = {
        aFinalCtorSet.mRenderpassCtorSet.mColorAttachment,
        aFinalCtorSet.mRenderpassCtorSet.mDepthAttachment
    };

    VkRenderPassCreateInfo renderPassInfo;{
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.flags = 0;
        renderPassInfo.attachmentCount = standardAttachments.size();
        renderPassInfo.pAttachments = standardAttachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &aFinalCtorSet.mRenderpassCtorSet.mSubpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &aFinalCtorSet.mRenderpassCtorSet.mDependency;
    }

    if(vkCreateRenderPass(aFinalCtorSet.mDevicePair.device, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS){
        throw std::runtime_error("Unable to create render pass!");
    }

    VkPipelineViewportStateCreateInfo viewportInfo;{
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.pNext = nullptr;
        viewportInfo.flags = 0;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &aFinalCtorSet.mViewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &aFinalCtorSet.mScissor;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo;{
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.flags = 0;
        pipelineInfo.stageCount = aFinalCtorSet.mProgrammableStages.size();
        pipelineInfo.pStages = aFinalCtorSet.mProgrammableStages.data();
        pipelineInfo.pVertexInputState = &aFinalCtorSet.mVtxInputInfo;
        pipelineInfo.pInputAssemblyState = &aFinalCtorSet.mInputAsmInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &aFinalCtorSet.mRasterInfo;
        pipelineInfo.pMultisampleState = &aFinalCtorSet.mMultisampleInfo;
        pipelineInfo.pDepthStencilState = &aFinalCtorSet.mDepthStencilInfo;
        pipelineInfo.pColorBlendState = &aFinalCtorSet.mColorBlendInfo;
        pipelineInfo.pDynamicState = aFinalCtorSet.mDynamicStates.empty() ? nullptr : &dynamicStateInfo;
        pipelineInfo.layout = mGraphicsPipeLayout;
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
    }

    if(vkCreateGraphicsPipelines(aFinalCtorSet.mDevicePair.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void VulkanBasicRasterPipelineBuilder::rebuild(){
    build(_mConstructionSet);
    fprintf(stderr, "Warning. VulkanBasicRasterPipelineBuilder::rebuild() not fully implemented!");
    throw std::runtime_error("TODO: Not implemented");
}

void VulkanBasicRasterPipelineBuilder::prepareFixedStages(GraphicsPipelineConstructionSet& aCtorSetInOut){
    {
        aCtorSetInOut.mVtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        aCtorSetInOut.mVtxInputInfo.pNext = nullptr;
        aCtorSetInOut.mVtxInputInfo.flags = 0;
        aCtorSetInOut.mVtxInputInfo.vertexBindingDescriptionCount = 0;
        aCtorSetInOut.mVtxInputInfo.pVertexAttributeDescriptions = nullptr;
        aCtorSetInOut.mVtxInputInfo.vertexAttributeDescriptionCount = 0;
        aCtorSetInOut.mVtxInputInfo.pVertexAttributeDescriptions = nullptr;
    }

    {
        aCtorSetInOut.mInputAsmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        aCtorSetInOut.mInputAsmInfo.pNext = nullptr;
        aCtorSetInOut.mInputAsmInfo.flags = 0;
        aCtorSetInOut.mInputAsmInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        aCtorSetInOut.mInputAsmInfo.primitiveRestartEnable = VK_FALSE;
    }

    {
        aCtorSetInOut.mRasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        aCtorSetInOut.mRasterInfo.pNext = nullptr;
        aCtorSetInOut.mRasterInfo.flags = 0;
        aCtorSetInOut.mRasterInfo.depthClampEnable = VK_FALSE;
        aCtorSetInOut.mRasterInfo.rasterizerDiscardEnable = VK_FALSE;
        aCtorSetInOut.mRasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        aCtorSetInOut.mRasterInfo.cullMode = VK_CULL_MODE_NONE;
        aCtorSetInOut.mRasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        aCtorSetInOut.mRasterInfo.depthBiasEnable = VK_FALSE;
        aCtorSetInOut.mRasterInfo.depthBiasConstantFactor = 0.0f;
        aCtorSetInOut.mRasterInfo.depthBiasClamp = 0.0f;
        aCtorSetInOut.mRasterInfo.depthBiasSlopeFactor = 0.0f;
        aCtorSetInOut.mRasterInfo.lineWidth = 1.0f;
    }

    {
        aCtorSetInOut.mMultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        aCtorSetInOut.mMultisampleInfo.pNext = nullptr;
        aCtorSetInOut.mMultisampleInfo.flags = 0;
        aCtorSetInOut.mMultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        aCtorSetInOut.mMultisampleInfo.sampleShadingEnable = VK_FALSE;
        aCtorSetInOut.mMultisampleInfo.minSampleShading = 1.0f;
        aCtorSetInOut.mMultisampleInfo.pSampleMask = nullptr;
        aCtorSetInOut.mMultisampleInfo.alphaToCoverageEnable = VK_FALSE;
        aCtorSetInOut.mMultisampleInfo.alphaToOneEnable = VK_FALSE;
    }

    {
        aCtorSetInOut.mBlendAttachmentInfo.blendEnable = VK_TRUE;
        aCtorSetInOut.mBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        aCtorSetInOut.mBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        aCtorSetInOut.mBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
        aCtorSetInOut.mBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        aCtorSetInOut.mBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        aCtorSetInOut.mBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
        aCtorSetInOut.mBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    {
        aCtorSetInOut.mColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        aCtorSetInOut.mColorBlendInfo.pNext = nullptr;
        aCtorSetInOut.mColorBlendInfo.flags = 0;
        aCtorSetInOut.mColorBlendInfo.logicOpEnable = VK_FALSE;
        aCtorSetInOut.mColorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
        aCtorSetInOut.mColorBlendInfo.attachmentCount = 1;
        aCtorSetInOut.mColorBlendInfo.pAttachments = &aCtorSetInOut.mBlendAttachmentInfo;
        aCtorSetInOut.mColorBlendInfo.blendConstants[0] = 0.0f;
        aCtorSetInOut.mColorBlendInfo.blendConstants[1] = 0.0f;
        aCtorSetInOut.mColorBlendInfo.blendConstants[2] = 0.0f;
        aCtorSetInOut.mColorBlendInfo.blendConstants[3] = 0.0f;
    }

    {
        bool depthExists = aCtorSetInOut.mDepthBundle.depthImage != VK_NULL_HANDLE;
        aCtorSetInOut.mDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        aCtorSetInOut.mDepthStencilInfo.pNext = nullptr;
        aCtorSetInOut.mDepthStencilInfo.flags = 0;
        aCtorSetInOut.mDepthStencilInfo.depthTestEnable = depthExists ? VK_TRUE : VK_FALSE;
        aCtorSetInOut.mDepthStencilInfo.depthWriteEnable = depthExists ? VK_TRUE : VK_FALSE;
        aCtorSetInOut.mDepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        aCtorSetInOut.mDepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        aCtorSetInOut.mDepthStencilInfo.stencilTestEnable = VK_FALSE;
        aCtorSetInOut.mDepthStencilInfo.front = {};
        aCtorSetInOut.mDepthStencilInfo.back = {};
        aCtorSetInOut.mDepthStencilInfo.minDepthBounds = 0.0f;
        aCtorSetInOut.mDepthStencilInfo.maxDepthBounds = 1.0f;
    }
}

void VulkanBasicRasterPipelineBuilder::prepareViewport(GraphicsPipelineConstructionSet& aCtorSetInOut){
    {
        aCtorSetInOut.mViewport.x = 0.0f;
        aCtorSetInOut.mViewport.y = 0.0f;
        aCtorSetInOut.mViewport.width = static_cast<float>(aCtorSetInOut.mSwapchainBundle->extent.width);
        aCtorSetInOut.mViewport.height = static_cast<float>(aCtorSetInOut.mSwapchainBundle->extent.height);
        aCtorSetInOut.mViewport.minDepth = 0.0f;
        aCtorSetInOut.mViewport.maxDepth = 1.0f;
    }

    {
        aCtorSetInOut.mScissor.offset = {0, 0};
        aCtorSetInOut.mScissor.extent = aCtorSetInOut.mSwapchainBundle->extent;
    }
}

void VulkanBasicRasterPipelineBuilder::prepareRenderPass(GraphicsPipelineConstructionSet& aCtorSetInOut){
    {
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.flags = 0;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.format = aCtorSetInOut.mSwapchainBundle->surface_format.format;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.format = aCtorSetInOut.mDepthBundle.format;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachmentRef.attachment = 0;
        aCtorSetInOut.mRenderpassCtorSet.mColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachmentRef.attachment = 1;
        aCtorSetInOut.mRenderpassCtorSet.mDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.flags = 0;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.inputAttachmentCount = 0;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pInputAttachments = nullptr;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.colorAttachmentCount = 1;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pColorAttachments = &aCtorSetInOut.mRenderpassCtorSet.mColorAttachmentRef;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pResolveAttachments = nullptr;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pDepthStencilAttachment = &aCtorSetInOut.mRenderpassCtorSet.mDepthAttachmentRef;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.preserveAttachmentCount = 0;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pPreserveAttachments = nullptr;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.dstSubpass = 0;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.srcAccessMask = 0;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        aCtorSetInOut.mRenderpassCtorSet.mDependency.dependencyFlags = 0;
    }
}

VulkanDepthBundle VulkanBasicRasterPipelineBuilder::autoCreateDepthBuffer(const GraphicsPipelineConstructionSet& aCtorSet){
    VulkanDepthBundle bundle;
    if(aCtorSet.mSwapchainBundle == nullptr){
        std::cerr << "Error: 'autoCreateDepthBuffer()' requires that a swapchain bundle is attached to the construction set." << std::endl;
        return(bundle);
    }

    bundle.format = vkutils::select_depth_format(aCtorSet.mDevicePair.physicalDevice);

    VkImageCreateInfo imageInfo = {};
    {
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.extent = VkExtent3D{aCtorSet.mSwapchainBundle->extent.width, aCtorSet.mSwapchainBundle->extent.height, 1};
        imageInfo.format = bundle.format;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.mipLevels = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.arrayLayers = 1;
    }

    VmaAllocationCreateInfo allocInfo = {};
    {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

    }

    VmaAllocator allocator = VmaHost::getAllocator({aCtorSet.mDevicePair.device, aCtorSet.mDevicePair.physicalDevice});
    if(vmaCreateImage(allocator, &imageInfo, &allocInfo, &bundle.depthImage, &bundle.mAllocation, &bundle.mAllocInfo) != VK_SUCCESS){
        throw std::runtime_error("Failed to create depth image!");
    }

    VkImageViewCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = bundle.depthImage;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = bundle.format;
        createInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        createInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    }
    if(vkCreateImageView(aCtorSet.mDevicePair.device, &createInfo, nullptr, &bundle.depthImageView) != VK_SUCCESS){
        throw std::runtime_error("Failed to create image view for depth buffer!");
    }
    
    return(bundle);
}

VulkanDepthBundle VulkanBasicRasterPipelineBuilder::autoCreateDepthBuffer() const{
    return(autoCreateDepthBuffer(_mConstructionSet));
}

} // end namespace vkutils