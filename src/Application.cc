#include "common.h"
#include "Application.h"
#include <iostream>
#include <cassert>

    
void Application::init(){
    BasicVulkanApp::init();
    initGraphicsPipeline();
}

void Application::run(){
    while(!glfwWindowShouldClose(mWindow)){

        glfwPollEvents();
    }
}

void Application::cleanup(){
    for(auto& [_, shader] : mShaderModules){
        vkDestroyShaderModule(mLogicalDevice.handle(), shader, nullptr);
    }
    vkDestroyPipeline(mLogicalDevice.handle(), mGraphicsPipeline, nullptr);
    vkDestroyRenderPass(mLogicalDevice.handle(), mRenderPass, nullptr);
    vkDestroyPipelineLayout(mLogicalDevice.handle(), mGraphicsPipeLayout, nullptr);
    BasicVulkanApp::cleanup();
}


void Application::initGraphicsPipeline(){
    VkShaderModule passthruShader = vkutils::load_shader_module(mLogicalDevice, "" STRIFY(SHADER_DIR) "/passthru.vert.spv"); 
    VkShaderModule fallbackShader = vkutils::load_shader_module(mLogicalDevice, STRIFY(SHADER_DIR) "/fallback.frag.spv");
    assert(passthruShader != VK_NULL_HANDLE);
    assert(fallbackShader != VK_NULL_HANDLE);
    mShaderModules.insert_or_assign("passthru.vert", passthruShader);
    mShaderModules.insert_or_assign("fallback.frag", fallbackShader);

    VkPipelineShaderStageCreateInfo vertStageInfo;{
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.pNext = nullptr;
        vertStageInfo.flags = 0;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = passthruShader;
        vertStageInfo.pName = "main";
        vertStageInfo.pSpecializationInfo = nullptr;
    }

    VkPipelineShaderStageCreateInfo fragStageInfo;{
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.pNext = nullptr;
        fragStageInfo.flags = 0;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fallbackShader;
        fragStageInfo.pName = "main";
        fragStageInfo.pSpecializationInfo = nullptr;
    }

    VkPipelineVertexInputStateCreateInfo vtxInputInfo;{
        vtxInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vtxInputInfo.pNext = nullptr;
        vtxInputInfo.flags = 0;
        vtxInputInfo.vertexBindingDescriptionCount = 0;
        vtxInputInfo.pVertexAttributeDescriptions = nullptr;
        vtxInputInfo.vertexAttributeDescriptionCount = 0;
        vtxInputInfo.pVertexAttributeDescriptions = nullptr;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAsmInfo;{
        inputAsmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsmInfo.pNext = nullptr;
        inputAsmInfo.flags = 0;
        inputAsmInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAsmInfo.primitiveRestartEnable = VK_FALSE;
    }

    VkViewport viewport;{
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(mSwapchainExtent.width);
        viewport.height = static_cast<float>(mSwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }

    VkRect2D scissor;{
        scissor.offset = {0, 0};
        scissor.extent = mSwapchainExtent;
    }

    VkPipelineViewportStateCreateInfo viewportInfo;{
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.pNext = nullptr;
        viewportInfo.flags = 0;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;
    }

    VkPipelineRasterizationStateCreateInfo rasterInfo;{
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.pNext = nullptr;
        rasterInfo.flags = 0;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_NONE;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.depthBiasConstantFactor = 0.0f;
        rasterInfo.depthBiasClamp = 0.0f;
        rasterInfo.depthBiasSlopeFactor = 0.0f;
        rasterInfo.lineWidth = 1.0f;
    }

    VkPipelineMultisampleStateCreateInfo multisampleInfo;{
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.pNext = nullptr;
        multisampleInfo.flags = 0;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        multisampleInfo.minSampleShading = 1.0f;
        multisampleInfo.pSampleMask = nullptr;
        multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleInfo.alphaToOneEnable = VK_FALSE;
    }

    VkPipelineColorBlendAttachmentState blendAttachmentInfo;{
        blendAttachmentInfo.blendEnable = VK_TRUE;
        blendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendInfo;{
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.pNext = nullptr;
        colorBlendInfo.flags = 0;
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &blendAttachmentInfo;
        colorBlendInfo.blendConstants[0] = 0.0f;
        colorBlendInfo.blendConstants[1] = 0.0f;
        colorBlendInfo.blendConstants[2] = 0.0f;
        colorBlendInfo.blendConstants[3] = 0.0f;
    }

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo;{
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pNext = nullptr;
        dynamicStateInfo.flags = 0;
        dynamicStateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateInfo.pDynamicStates = dynamicStates.data();
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;{
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pNext = nullptr;
        pipelineLayoutInfo.flags = 0;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    vkCreatePipelineLayout(mLogicalDevice.handle(), &pipelineLayoutInfo, nullptr, &mGraphicsPipeLayout);
    
    initRenderPasses();
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertStageInfo, fragStageInfo};
    VkGraphicsPipelineCreateInfo pipelineInfo;{
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.flags = 0;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vtxInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAsmInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.layout = mGraphicsPipeLayout;
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
    }

    if(vkCreateGraphicsPipelines(mLogicalDevice.handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

}

void Application::initRenderPasses(){
    VkAttachmentDescription colorAttachment;{
        colorAttachment.flags = 0;
        colorAttachment.format = mSwapchainFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    VkAttachmentReference attachmentRef;{
        attachmentRef.attachment = 0;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass;{
        subpass.flags = 0;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRef;
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
    }

    VkRenderPassCreateInfo renderPassInfo;{
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.flags = 0;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;
    }

    if(vkCreateRenderPass(mLogicalDevice.handle(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS){
        throw std::runtime_error("Unable to create render pass!");
    }
}