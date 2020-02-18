#include "utils/common.h"
#include "vkutils/vkutils.h"
#include "VulkanGraphicsApp.h"
#include "data/VertexInput.h"
#include <glm/glm.hpp>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

    
void VulkanGraphicsApp::init(){
    initUniformBuffer();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();
}

const VkExtent2D& VulkanGraphicsApp::getFramebufferSize() const{
    return(mViewportExtent);
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

void VulkanGraphicsApp::setVertexShader(const std::string& aShaderName, const VkShaderModule& aShaderModule){
    if(aShaderName.empty() || aShaderModule == VK_NULL_HANDLE){
        throw std::runtime_error("VulkanGraphicsApp::setVertexShader() Error: Arguments must be a non-empty string and valid shader module!");
    }
    mShaderModules[aShaderName] = aShaderModule;
    mVertexKey = aShaderName;
    if(mVertexKey == mFragmentKey){
        throw std::runtime_error("Error: Keys/Names for the vertex and fragment shader cannot be the same!");
    }
}

void VulkanGraphicsApp::setFragmentShader(const std::string& aShaderName, const VkShaderModule& aShaderModule){
    if(aShaderName.empty() || aShaderModule == VK_NULL_HANDLE){
        throw std::runtime_error("VulkanGraphicsApp::setFragmentShader() Error: Arguments must be a non-empty string and valid shader module!");
    }
    mShaderModules[aShaderName] = aShaderModule;
    mFragmentKey = aShaderName;
    if(mVertexKey == mFragmentKey){
        throw std::runtime_error("Error: Keys/Names for the vertex and fragment shader cannot be the same!");
    }
}

void VulkanGraphicsApp::addUniform(uint32_t aBindingPoint, UniformDataInterfacePtr aUniformData, VkShaderStageFlags aStages){
    if(aUniformData == nullptr){
        std::cerr << "Ignoring attempt to add nullptr as uniform data!" << std::endl;
        return;
    }

    mUniformBuffer.bindUniformData(aBindingPoint, aUniformData, aStages);

    if(mRenderPipeline.isValid())
        resetRenderSetup();
}

void VulkanGraphicsApp::resetRenderSetup(){
    vkDeviceWaitIdle(mDeviceBundle.logicalDevice.handle());

    cleanupSwapchainDependents();
    VulkanSetupBaseApp::cleanupSwapchain();

    VulkanSetupBaseApp::initSwapchain();
    initUniformBuffer();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();

    sWindowFlags[mWindow].resized = false;
}

void VulkanGraphicsApp::render(){
    uint32_t targetImageIndex = 0;
    size_t syncObjectIndex = mFrameNumber % IN_FLIGHT_FRAME_LIMIT;

    vkWaitForFences(mDeviceBundle.logicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(mDeviceBundle.logicalDevice.handle(),
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

    vkResetFences(mDeviceBundle.logicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex]);

    
    mUniformBuffer.updateDevice();

    if(vkQueueSubmit(mDeviceBundle.logicalDevice.getGraphicsQueue(), 1, &submitInfo, mInFlightFences[syncObjectIndex]) != VK_SUCCESS){
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

    vkQueuePresentKHR(mDeviceBundle.logicalDevice.getPresentationQueue(), &presentInfo);
}

void VulkanGraphicsApp::initRenderPipeline(){
    if(!mVertexInputsHaveBeenSet){
        throw std::runtime_error("Error! Render pipeline cannot be created before vertex input information has been set via 'setVertexInput()'");
    }else if(mVertexKey.empty()){
        throw std::runtime_error("Error! No vertex shader has been set! A vertex shader must be set using setVertexShader()!");
    }else if(mFragmentKey.empty()){
        throw std::runtime_error("Error! No fragment shader has been set! A vertex shader must be set using setFragmentShader()!");
    }

    vkutils::GraphicsPipelineConstructionSet& ctorSet =  mRenderPipeline.setupConstructionSet(mDeviceBundle.logicalDevice.handle(), &mSwapchainBundle);
    vkutils::BasicVulkanRenderPipeline::prepareFixedStages(ctorSet);

    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
    auto findVert = mShaderModules.find(mVertexKey);
    auto findFrag = mShaderModules.find(mFragmentKey);
    if(findVert == mShaderModules.end()){
        throw std::runtime_error("Error: Vertex shader name '" + mVertexKey + "' did not map to a valid shader module");
    }else{
        vertShader = findVert->second;
    }
    if(findFrag == mShaderModules.end()){
        std::cerr << "Error: Fragment shader name '" + mFragmentKey + "' did not map to a valid shader module. Using fallback..." << std::endl;
        findFrag = mShaderModules.find("fallback.frag");
        if(findFrag != mShaderModules.end()){
            fragShader = findFrag->second;
        }else{
            fragShader = vkutils::load_shader_module(mDeviceBundle.logicalDevice.handle(), STRIFY(SHADER_DIR) "/fallback.frag.spv");
            mShaderModules["fallback.frag"] = fragShader;
        }
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

    ctorSet.mPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ctorSet.mPipelineLayoutInfo.pNext = 0;
    ctorSet.mPipelineLayoutInfo.flags = 0;
    ctorSet.mPipelineLayoutInfo.setLayoutCount = mUniformDescriptorSetLayouts.size();
    ctorSet.mPipelineLayoutInfo.pSetLayouts = mUniformDescriptorSetLayouts.data();
    ctorSet.mPipelineLayoutInfo.pushConstantRangeCount = 0;
    ctorSet.mPipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkutils::BasicVulkanRenderPipeline::prepareViewport(ctorSet);
    vkutils::BasicVulkanRenderPipeline::prepareRenderPass(ctorSet);
    mRenderPipeline.build(ctorSet);
}

void VulkanGraphicsApp::initCommands(){
    VkCommandPoolCreateInfo poolInfo;{
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = 0;
        poolInfo.queueFamilyIndex = *mDeviceBundle.physicalDevice.mGraphicsIdx;
    }

    if(mCommandPool == VK_NULL_HANDLE){
        if(vkCreateCommandPool(mDeviceBundle.logicalDevice.handle(), &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS){
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
    if(vkAllocateCommandBuffers(mDeviceBundle.logicalDevice.handle(), &allocInfo, mCommandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for(size_t i = 0; i < mCommandBuffers.size(); ++i){
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0 , nullptr};
        if(vkBeginCommandBuffer(mCommandBuffers[i], &beginInfo) != VK_SUCCESS){
            throw std::runtime_error("Failed to begine command recording!");
        }

        static const VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
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

        // Bind uniforms to graphics pipeline if they exist
        if(mUniformBuffer.getBoundDataCount() > 0){
            const VkDescriptorSet* descriptorSets = mUniformDescriptorSets.data() + i;
            vkCmdBindDescriptorSets(
                mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipeline.getLayout(),
                0, 1, descriptorSets, 0, nullptr
            );
        }

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

        if(vkCreateFramebuffer(mDeviceBundle.logicalDevice.handle(), &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS){
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
        failure |= vkCreateSemaphore(mDeviceBundle.logicalDevice.handle(), &semaphoreCreate, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateSemaphore(mDeviceBundle.logicalDevice.handle(), &semaphoreCreate, nullptr, &mRenderFinishSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateFence(mDeviceBundle.logicalDevice.handle(), &fenceInfo, nullptr, &mInFlightFences[i]);
    }
    if(failure){
        throw std::runtime_error("Failed to create semaphores!");
    }

}

void VulkanGraphicsApp::cleanupSwapchainDependents(){
    vkDestroyDescriptorPool(mDeviceBundle.logicalDevice, mUniformDescriptorPool, nullptr);

    for(size_t i = 0; i < IN_FLIGHT_FRAME_LIMIT; ++i){
        vkDestroySemaphore(mDeviceBundle.logicalDevice.handle(), mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mDeviceBundle.logicalDevice.handle(), mRenderFinishSemaphores[i], nullptr);
        vkDestroyFence(mDeviceBundle.logicalDevice.handle(), mInFlightFences[i], nullptr);
    }

    vkFreeCommandBuffers(mDeviceBundle.logicalDevice.handle(), mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());
    
    for(const VkFramebuffer& fb : mSwapchainFramebuffers){
        vkDestroyFramebuffer(mDeviceBundle.logicalDevice.handle(), fb, nullptr);
    }

    mRenderPipeline.destroy();
}

void VulkanGraphicsApp::cleanup(){
    for(std::pair<const std::string, VkShaderModule>& module : mShaderModules){
        vkDestroyShaderModule(mDeviceBundle.logicalDevice.handle(), module.second, nullptr);
    }

    cleanupSwapchainDependents();

    mUniformBuffer.freeBuffer();
    mUniformDescriptorSets.clear();

    vkDestroyCommandPool(mDeviceBundle.logicalDevice.handle(), mCommandPool, nullptr);

    VulkanSetupBaseApp::cleanup();
}

void VulkanGraphicsApp::initUniformBuffer() {
    if(mUniformBuffer.getBoundDataCount() == 0) return;
    
    if(!mUniformBuffer.getCurrentDevice().isValid()){
        mUniformBuffer.updateDevice(mDeviceBundle);
    }else{
        mUniformBuffer.updateDevice();
    }

    mTotalUniformDescriptorSetCount = mSwapchainBundle.images.size();
    mUniformDescriptorSetLayouts.assign(1, mUniformBuffer.getDescriptorSetLayout());

    initUniformDescriptorPool();
    initUniformDescriptorSets();
}
    
void VulkanGraphicsApp::initUniformDescriptorPool() {
    VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = mTotalUniformDescriptorSetCount*mUniformBuffer.getBoundDataCount(); // TODO: Check for error

    VkDescriptorPoolCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.maxSets = mTotalUniformDescriptorSetCount;
        createInfo.poolSizeCount = 1;
        createInfo.pPoolSizes = &poolSize;
    }

    if(vkCreateDescriptorPool(mDeviceBundle.logicalDevice, &createInfo, nullptr, &mUniformDescriptorPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to create uniform descriptor pool");
    }
}
    
void VulkanGraphicsApp::initUniformDescriptorSets() {
    assert(mTotalUniformDescriptorSetCount != 0);

    
    std::vector<VkDescriptorSetLayout> layouts(mTotalUniformDescriptorSetCount, mUniformBuffer.getDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfo;
    {
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = mUniformDescriptorPool;
        allocInfo.descriptorSetCount = mTotalUniformDescriptorSetCount;
        allocInfo.pSetLayouts = layouts.data();
    }
    
    mUniformDescriptorSets.resize(mTotalUniformDescriptorSetCount, VK_NULL_HANDLE);
    VkResult allocResult = vkAllocateDescriptorSets(mDeviceBundle.logicalDevice, &allocInfo, mUniformDescriptorSets.data());
    if(allocResult != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate uniform descriptor sets");
    }

    std::vector<VkDescriptorBufferInfo> bufferInfos = mUniformBuffer.getDescriptorBufferInfos();
    std::vector<uint32_t> bindingPoints = mUniformBuffer.getBoundPoints();

    std::vector<VkWriteDescriptorSet> setWriters;
    setWriters.reserve(mUniformDescriptorSets.size() * bindingPoints.size());

    for(VkDescriptorSet descriptorSet : mUniformDescriptorSets){
        for(size_t i = 0; i < bindingPoints.size(); ++i){
            setWriters.emplace_back(
                VkWriteDescriptorSet{
                    /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    /* pNext = */ nullptr, 
                    /* dstSet = */ descriptorSet,
                    /* dstBinding = */ bindingPoints[i],
                    /* dstArrayElement = */ 0,
                    /* descriptorCount = */ 1,
                    /* descriptorType = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    /* pImageInfo = */ nullptr,
                    /* pBufferInfo = */ &bufferInfos[i],
                    /* pTexelBufferView = */ nullptr
                }
            );
        }
    }
    vkUpdateDescriptorSets(mDeviceBundle.logicalDevice, setWriters.size(), setWriters.data(), 0, nullptr);
}

