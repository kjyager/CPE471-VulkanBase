#include "vkutils.h"
#include <iostream>

namespace vkutils{

VulkanComputePipeline VulkanComputePipelineBuilder::build(VkDevice aLogicalDevice){
    if(vkCreatePipelineLayout(aLogicalDevice, &mCtorSet.mLayoutInfo, nullptr, &mLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed when creating compute pipeline layout!");
    }

    mCtorSet.mComputePipelineInfo.layout = mLayout;

    if(vkCreateComputePipelines(aLogicalDevice, VK_NULL_HANDLE, 1, &mCtorSet.mComputePipelineInfo, nullptr, &mPipeline) != VK_SUCCESS){
        throw std::runtime_error("Failed when creating compute pipeline!");
    }

    return(*this);
}

void VulkanComputePipeline::destroy(VkDevice aLogicalDevice){
    if(!_isValid()){
        std::cerr << "Warning! Cannot destroy VulkanComputePipeline because it hasn't been built" << std::endl;
        return;
    }

    vkDestroyPipelineLayout(aLogicalDevice, mLayout, nullptr);
    mLayout = VK_NULL_HANDLE;

    vkDestroyPipeline(aLogicalDevice, mPipeline, nullptr);
    mPipeline = VK_NULL_HANDLE;
}

void VulkanComputePipelineBuilder::prepareUnspecialized(ComputePipelineConstructionSet& aCtorSet, VkShaderModule aComputeModule){
    VkPipelineShaderStageCreateInfo stageInfo = {};
    {
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.flags = 0;
        stageInfo.module = aComputeModule;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.pName = "main";
        stageInfo.pSpecializationInfo = nullptr;
    }

    VulkanComputePipelineBuilder::prepareWithStage(aCtorSet, stageInfo);
}

void VulkanComputePipelineBuilder::prepareWithStage(ComputePipelineConstructionSet& aCtorSet, const VkPipelineShaderStageCreateInfo& aComputeStage){
    aCtorSet.mLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    aCtorSet.mLayoutInfo.pNext = nullptr;
    aCtorSet.mLayoutInfo.flags = 0;
    aCtorSet.mLayoutInfo.pushConstantRangeCount = 0;
    aCtorSet.mLayoutInfo.setLayoutCount = 0;

    aCtorSet.mComputePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    aCtorSet.mComputePipelineInfo.pNext = nullptr;
    aCtorSet.mComputePipelineInfo.flags = 0;
    aCtorSet.mComputePipelineInfo.layout = VK_NULL_HANDLE;
    aCtorSet.mComputePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    aCtorSet.mComputePipelineInfo.basePipelineIndex = 0;
    aCtorSet.mComputePipelineInfo.stage = aComputeStage;
}

}