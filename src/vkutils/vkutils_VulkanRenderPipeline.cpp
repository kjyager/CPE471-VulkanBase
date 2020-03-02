#include "vkutils.h"
#include <cassert>

namespace vkutils
{

BasicVulkanRenderPipeline::BasicVulkanRenderPipeline(const VkDevice& aLogicalDevice, const VulkanSwapchainBundle* aChainBundle)
:   _mConstructionSet(aLogicalDevice, aChainBundle), _mLogicalDevice(aLogicalDevice)
{}

void BasicVulkanRenderPipeline::destroy(){
    vkDestroyPipeline(_mLogicalDevice, mGraphicsPipeline, nullptr);
    vkDestroyRenderPass(_mLogicalDevice, mRenderPass, nullptr);
    vkDestroyPipelineLayout(_mLogicalDevice, mGraphicsPipeLayout, nullptr);
    _mValid = false;
}

GraphicsPipelineConstructionSet& BasicVulkanRenderPipeline::setupConstructionSet(const VkDevice& aLogicalDevice, const VulkanSwapchainBundle* aChainBundle){
    _mLogicalDevice = aLogicalDevice;
    _mConstructionSet = GraphicsPipelineConstructionSet(aLogicalDevice, aChainBundle);
    return(_mConstructionSet);
}

void BasicVulkanRenderPipeline::build(const GraphicsPipelineConstructionSet& aFinalCtorSet){
    if(_mLogicalDevice != aFinalCtorSet.mLogicalDevice){
        throw std::runtime_error("Logical device assigned to BasicVulkanRenderPipeline does not match the device in the constructions set.");
    }
    _mConstructionSet = aFinalCtorSet;
    
    // Create pipeline layout object
    vkCreatePipelineLayout(aFinalCtorSet.mLogicalDevice, &aFinalCtorSet.mPipelineLayoutInfo, nullptr, &mGraphicsPipeLayout);

    VkPipelineDynamicStateCreateInfo dynamicStateInfo;{
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pNext = nullptr;
        dynamicStateInfo.flags = 0;
        dynamicStateInfo.dynamicStateCount = aFinalCtorSet.mDynamicStates.size();
        dynamicStateInfo.pDynamicStates = aFinalCtorSet.mDynamicStates.data();
    }

    VkRenderPassCreateInfo renderPassInfo;{
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.flags = 0;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &aFinalCtorSet.mRenderpassCtorSet.mColorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &aFinalCtorSet.mRenderpassCtorSet.mSubpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &aFinalCtorSet.mRenderpassCtorSet.mDependency;
    }

    if(vkCreateRenderPass(aFinalCtorSet.mLogicalDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS){
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
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &aFinalCtorSet.mColorBlendInfo;
        pipelineInfo.pDynamicState = aFinalCtorSet.mDynamicStates.empty() ? nullptr : &dynamicStateInfo;
        pipelineInfo.layout = mGraphicsPipeLayout;
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
    }

    if(vkCreateGraphicsPipelines(aFinalCtorSet.mLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    _mValid = true;
}

void BasicVulkanRenderPipeline::rebuild(){
    build(_mConstructionSet);
    fprintf(stderr, "Warning. BasicVulkanRenderPipeline::rebuild() not fully implemented!");
    // throw std::runtime_error("TODO: Not implemented");
}

void BasicVulkanRenderPipeline::prepareFixedStages(GraphicsPipelineConstructionSet& aCtorSetInOut){
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
}

void BasicVulkanRenderPipeline::prepareViewport(GraphicsPipelineConstructionSet& aCtorSetInOut){
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

void BasicVulkanRenderPipeline::prepareRenderPass(GraphicsPipelineConstructionSet& aCtorSetInOut){
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
        aCtorSetInOut.mRenderpassCtorSet.mAttachmentRef.attachment = 0;
        aCtorSetInOut.mRenderpassCtorSet.mAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    {
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.flags = 0;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.inputAttachmentCount = 0;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pInputAttachments = nullptr;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.colorAttachmentCount = 1;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pColorAttachments = &aCtorSetInOut.mRenderpassCtorSet.mAttachmentRef;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pResolveAttachments = nullptr;
        aCtorSetInOut.mRenderpassCtorSet.mSubpass.pDepthStencilAttachment = nullptr;
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

} // end namespace vkutils