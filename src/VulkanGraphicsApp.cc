#include "utils/common.h"
#include "VulkanGraphicsApp.h"
#include "data/VertexInput.h"
#include <glm/glm.hpp>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

    
void VulkanGraphicsApp::init(){
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();
}

void VulkanGraphicsApp::setVertexInput(
    const VkVertexInputBindingDescription& aBindingDescription,
    const std::vector<VkVertexInputAttributeDescription>& aAttributeDescriptions
){
    mBindingDescription = aBindingDescription;
    mAttributeDescriptions = aAttributeDescriptions;
    if(mVertexInputsHaveBeenSet){
        //TODO: Verify this works 
        resetRenderSetup();
    }
    mVertexInputsHaveBeenSet = true;
}

void VulkanGraphicsApp::setVertexBuffer(const VkBuffer& aBuffer, size_t aVertexCount){
    bool needsReset = mVertexBuffer != VK_NULL_HANDLE && (mVertexBuffer != aBuffer || mVertexCount != aVertexCount); 
    mVertexBuffer = aBuffer;
    mVertexCount = aVertexCount;
    if(needsReset) resetRenderSetup(); // TODO: Verify 
}

void VulkanGraphicsApp::resetRenderSetup(){
    vkDeviceWaitIdle(mLogicalDevice.handle());

    cleanupSwapchainDependents();
    VulkanSetupBaseApp::cleanupSwapchain();

    VulkanSetupBaseApp::initSwapchain();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();

    sWindowFlags[mWindow].resized = false;
}

void VulkanGraphicsApp::render(){
    uint32_t targetImageIndex = 0;
    size_t syncObjectIndex = mFrameNumber % IN_FLIGHT_FRAME_LIMIT;

    vkWaitForFences(mLogicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(mLogicalDevice.handle(),
        mSwapchainBundle.swapchain, std::numeric_limits<uint64_t>::max(),
        mImageAvailableSemaphores[syncObjectIndex], VK_NULL_HANDLE, &targetImageIndex
    );
    if(result == VK_ERROR_OUT_OF_DATE_KHR || sWindowFlags[mWindow].resized){
        resetRenderSetup();
        render();
        return;
    }else if(result == VK_SUBOPTIMAL_KHR){
        std::cerr << "Warning! Swapchain suboptimal" << std::endl;
    }else if(result != VK_SUCCESS){
        throw std::runtime_error("Failed to get next image in swapchain!");
    }

    const static VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr,
        1, &mImageAvailableSemaphores[syncObjectIndex], &waitStages,
        1, &mCommandBuffers[targetImageIndex],
        1, &mRenderFinishSemaphores[syncObjectIndex]
    };

    vkResetFences(mLogicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex]);

    if(vkQueueSubmit(mLogicalDevice.getGraphicsQueue(), 1, &submitInfo, mInFlightFences[syncObjectIndex]) != VK_SUCCESS){
        throw std::runtime_error("Submit to graphics queue failed!");
    }

    VkPresentInfoKHR presentInfo = {
        /*sType = */ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        /*pNext = */ nullptr,
        /*waitSemaphoreCount = */ 1,
        /*pWaitSemaphores = */ &mRenderFinishSemaphores[syncObjectIndex],
        /*swapchainCount = */ 1,
        /*pSwapchains = */ &mSwapchainBundle.swapchain,
        /*pImageIndices = */ &targetImageIndex,
        /*pResults = */ nullptr
    };

    vkQueuePresentKHR(mLogicalDevice.getPresentationQueue(), &presentInfo);
}

void VulkanGraphicsApp::cleanup(){
    for(std::pair<const std::string, VkShaderModule>& module : mShaderModules){
        vkDestroyShaderModule(mLogicalDevice.handle(), module.second, nullptr);
    }
    VulkanGraphicsApp::cleanupSwapchainDependents();
    vkDestroyCommandPool(mLogicalDevice.handle(), mCommandPool, nullptr);
    VulkanSetupBaseApp::cleanup();
}

void VulkanGraphicsApp::cleanupSwapchainDependents(){
    for(size_t i = 0; i < IN_FLIGHT_FRAME_LIMIT; ++i){
        vkDestroySemaphore(mLogicalDevice.handle(), mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mLogicalDevice.handle(), mRenderFinishSemaphores[i], nullptr);
        vkDestroyFence(mLogicalDevice.handle(), mInFlightFences[i], nullptr);
    }

    vkFreeCommandBuffers(mLogicalDevice.handle(), mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());
    
    for(const VkFramebuffer& fb : mSwapchainFramebuffers){
        vkDestroyFramebuffer(mLogicalDevice.handle(), fb, nullptr);
    }

    mRenderPipeline.destroy();
}


void VulkanGraphicsApp::initRenderPipeline(){
    if(!mVertexInputsHaveBeenSet){
        throw std::runtime_error("Error! Render pipeline cannot be created before vertex input information has been set via 'setVertexInput()'");
    }

    vkutils::GraphicsPipelineConstructionSet& ctorSet =  mRenderPipeline.setupConstructionSet(mLogicalDevice.handle(), &mSwapchainBundle);
    vkutils::BasicVulkanRenderPipeline::prepareFixedStages(ctorSet);

    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
    auto findVert = mShaderModules.find("passthru.vert");
    auto findFrag = mShaderModules.find("vertexColor.frag");
    if(findVert == mShaderModules.end()){
        vertShader = vkutils::load_shader_module(mLogicalDevice.handle(), STRIFY(SHADER_DIR) "/passthru.vert.spv");
        mShaderModules["passthru.vert"] = vertShader;
    }else{
        vertShader = findVert->second;
    }
    if(findFrag == mShaderModules.end()){
        fragShader = vkutils::load_shader_module(mLogicalDevice.handle(), STRIFY(SHADER_DIR) "/vertexColor.frag.spv");
        mShaderModules["vertexColor.frag"] = fragShader;
    }else{
        fragShader = findFrag->second;
    }
    
    VkPipelineShaderStageCreateInfo vertStageInfo;{
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.pNext = nullptr;
        vertStageInfo.flags = 0;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertShader;
        vertStageInfo.pName = "main";
        vertStageInfo.pSpecializationInfo = nullptr;
    }
    VkPipelineShaderStageCreateInfo fragStageInfo;{
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.pNext = nullptr;
        fragStageInfo.flags = 0;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShader;
        fragStageInfo.pName = "main";
        fragStageInfo.pSpecializationInfo = nullptr;
    }
    ctorSet.mProgrammableStages.emplace_back(vertStageInfo);
    ctorSet.mProgrammableStages.emplace_back(fragStageInfo);

    ctorSet.mVtxInputInfo.pVertexBindingDescriptions = &mBindingDescription;
    ctorSet.mVtxInputInfo.vertexBindingDescriptionCount = 1U;
    ctorSet.mVtxInputInfo.pVertexAttributeDescriptions = mAttributeDescriptions.data();
    ctorSet.mVtxInputInfo.vertexAttributeDescriptionCount = mAttributeDescriptions.size();

    vkutils::BasicVulkanRenderPipeline::prepareViewport(ctorSet);
    vkutils::BasicVulkanRenderPipeline::prepareRenderPass(ctorSet);
    mRenderPipeline.build(ctorSet);
}

void VulkanGraphicsApp::initCommands(){
    VkCommandPoolCreateInfo poolInfo;{
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = 0;
        poolInfo.queueFamilyIndex = *mPhysDevice.mGraphicsIdx;
    }

    if(mCommandPool == VK_NULL_HANDLE){
        if(vkCreateCommandPool(mLogicalDevice.handle(), &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create command pool for graphics queue!");
        }
    }

    mCommandBuffers.resize(mSwapchainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo;{
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.commandBufferCount = mCommandBuffers.size();
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }
    if(vkAllocateCommandBuffers(mLogicalDevice.handle(), &allocInfo, mCommandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for(size_t i = 0; i < mCommandBuffers.size(); ++i){
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0 , nullptr};
        if(vkBeginCommandBuffer(mCommandBuffers[i], &beginInfo) != VK_SUCCESS){
            throw std::runtime_error("Failed to begine command recording!");
        }

        static const VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkRenderPassBeginInfo renderBegin;{
            renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderBegin.pNext = nullptr;
            renderBegin.renderPass = mRenderPipeline.getRenderpass();
            renderBegin.framebuffer = mSwapchainFramebuffers[i];
            renderBegin.renderArea = {{0,0}, mSwapchainBundle.extent};
            renderBegin.clearValueCount = 1;
            renderBegin.pClearValues = &clearColor;
        }

        vkCmdBeginRenderPass(mCommandBuffers[i], &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipeline.getPipeline());
        vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1, &mVertexBuffer, std::array<VkDeviceSize, 1>{0}.data());
        vkCmdDraw(mCommandBuffers[i], mVertexCount, 1, 0, 0);
        vkCmdEndRenderPass(mCommandBuffers[i]);

        if(vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to end command buffer " + std::to_string(i));
        }
    }
}

void VulkanGraphicsApp::initFramebuffers(){
    mSwapchainFramebuffers.resize(mSwapchainBundle.views.size());
    for(size_t i = 0; i < mSwapchainBundle.views.size(); ++i){
        VkFramebufferCreateInfo framebufferInfo;{
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.pNext = nullptr;
            framebufferInfo.flags = 0;
            framebufferInfo.renderPass = mRenderPipeline.getRenderpass();
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &mSwapchainBundle.views[i];
            framebufferInfo.width = mSwapchainBundle.extent.width;
            framebufferInfo.height = mSwapchainBundle.extent.height;
            framebufferInfo.layers = 1;
        }

        if(vkCreateFramebuffer(mLogicalDevice.handle(), &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to create swap chain framebuffer " + std::to_string(i));
        }
    }
}


void VulkanGraphicsApp::initSync(){
    mImageAvailableSemaphores.resize(IN_FLIGHT_FRAME_LIMIT);
    mRenderFinishSemaphores.resize(IN_FLIGHT_FRAME_LIMIT);
    mInFlightFences.resize(IN_FLIGHT_FRAME_LIMIT);

    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
    VkSemaphoreCreateInfo semaphoreCreate = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

    bool failure = false;
    for(size_t i = 0 ; i < IN_FLIGHT_FRAME_LIMIT; ++i){
        failure |= vkCreateSemaphore(mLogicalDevice.handle(), &semaphoreCreate, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateSemaphore(mLogicalDevice.handle(), &semaphoreCreate, nullptr, &mRenderFinishSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateFence(mLogicalDevice.handle(), &fenceInfo, nullptr, &mInFlightFences[i]);
    }
    if(failure){
        throw std::runtime_error("Failed to create semaphores!");
    }

}
