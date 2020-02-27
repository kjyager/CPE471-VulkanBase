#include "utils/common.h"
#include "vkutils/vkutils.h"
#include "VulkanGraphicsApp.h"
#include "data/VertexInput.h"
#include "utils/map_merge.h"
#include "utils/BufferedTimer.h"
#include "vkutils/VmaHost.h"
#include <glm/glm.hpp>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <numeric>

void VulkanGraphicsApp::init(){
    if(mCoreProvider == nullptr){
        initCore();
    }

    QUICK_TIME("initCommandPool", initCommandPool());
    QUICK_TIME("initTransferCmdBuffer", initTransferCmdBuffer());
    QUICK_TIME("transferGeometry", transferGeometry());
    QUICK_TIME("initUniformResources", initUniformResources());
    QUICK_TIME("initRenderPipeline", initRenderPipeline());
    QUICK_TIME("initFramebuffers", initFramebuffers());
    QUICK_TIME("initCommands", initCommands());
    QUICK_TIME("initSync", initSync());
}

const VkExtent2D& VulkanGraphicsApp::getFramebufferSize() const{
    return(mSwapchainProvider->getPresentationExtent());
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

void VulkanGraphicsApp::initMultiShapeUniformBuffer(const UniformDataLayoutSet& aUniformLayout){
    mMultiUniformBuffer = std::make_shared<MultiInstanceUniformBuffer>(
        getPrimaryDeviceBundle(),
        aUniformLayout,
        0, // Instances to start with
        16 // Capacity to start with
    );
}

void VulkanGraphicsApp::addMultiShapeObject(const ObjMultiShapeGeometry& mObject, const UniformDataInterfaceSet& aUniformData){
    mMultiShapeObjects.emplace_back(mObject);
    if(mMultiUniformBuffer == nullptr){
        throw std::runtime_error("initMultiShapeUniformBuffer() must be called before addMultiShapeObject()!");
    }
    mMultiUniformBuffer->pushBackInstance(aUniformData);

    if(mTransferCmdBuffer != VK_NULL_HANDLE){
        transferGeometry();
        reinitUniformResources();
    }
}

void VulkanGraphicsApp::addSingleInstanceUniform(uint32_t aBindPoint, const UniformDataInterfacePtr& aUniformInterface){
    if(!mSingleUniformBuffer.getCurrentDevice().isValid()){
        throw std::runtime_error("Single instance uniforms cannot be added because the uniform buffer has not been initialized");
    }
    mSingleUniformBuffer.bindUniformData(aBindPoint, aUniformInterface);
    reinitUniformResources();
}

const VkApplicationInfo& VulkanGraphicsApp::getAppInfo() const {
    const static VkApplicationInfo appInfo{
        /* sType = */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* pNext = */ nullptr,
        /* pApplicationName = */ "CPE 471 MultiShape Scene",
        /* applicationVersion = */ VK_MAKE_VERSION(0, 0, 0),
        /* pEngineName = */ "471W20 OBJ base code",
        /* engineVersion = */ VK_MAKE_VERSION(0, 0, 0),
        /* apiVersion = */ VK_API_VERSION_1_1
    };
    return(appInfo);
}

const std::vector<std::string>& VulkanGraphicsApp::getRequestedValidationLayers() const{
    const static std::vector<std::string> sLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor"};
    return(sLayers);
}

void VulkanGraphicsApp::resetRenderSetup(){
    vkDeviceWaitIdle(getPrimaryDeviceBundle().logicalDevice.handle());

    cleanupSwapchainDependents();
    mSwapchainProvider->cleanupSwapchain();

    mSwapchainProvider->initSwapchain();
    initUniformResources();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();

    SwapchainProvider::sWindowFlags[mSwapchainProvider->getWindowPtr()].resized = false;
}

void VulkanGraphicsApp::render(){
    uint32_t targetImageIndex = 0;
    size_t syncObjectIndex = mFrameNumber % IN_FLIGHT_FRAME_LIMIT;

    vkWaitForFences(getPrimaryDeviceBundle().logicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(getPrimaryDeviceBundle().logicalDevice.handle(),
        mSwapchainProvider->getSwapchainBundle().swapchain, std::numeric_limits<uint64_t>::max(),
        mImageAvailableSemaphores[syncObjectIndex], VK_NULL_HANDLE, &targetImageIndex
    );

    GLFWwindow* window = mSwapchainProvider->getWindowPtr();
    if(result == VK_ERROR_OUT_OF_DATE_KHR || SwapchainProvider::sWindowFlags[window].resized){
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

    vkResetFences(getPrimaryDeviceBundle().logicalDevice.handle(), 1, &mInFlightFences[syncObjectIndex]);
    
    mMultiUniformBuffer->updateDevice();
    mSingleUniformBuffer.updateDevice();

    if(vkQueueSubmit(getPrimaryDeviceBundle().logicalDevice.getGraphicsQueue(), 1, &submitInfo, mInFlightFences[syncObjectIndex]) != VK_SUCCESS){
        throw std::runtime_error("Submit to graphics queue failed!");
    }

    VkPresentInfoKHR presentInfo = {
        /*sType = */ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        /*pNext = */ nullptr,
        /*waitSemaphoreCount = */ 1,
        /*pWaitSemaphores = */ &mRenderFinishSemaphores[syncObjectIndex],
        /*swapchainCount = */ 1,
        /*pSwapchains = */ &mSwapchainProvider->getSwapchainBundle().swapchain,
        /*pImageIndices = */ &targetImageIndex,
        /*pResults = */ nullptr
    };

    vkQueuePresentKHR(getPrimaryDeviceBundle().logicalDevice.getPresentationQueue(), &presentInfo);

    ++mFrameNumber;
}

void VulkanGraphicsApp::initCore(){
    mCoreProvider = std::make_shared<VulkanSetupCore>();
    CoreLink::mCoreProvider = mCoreProvider.get();
    mSwapchainProvider = std::make_shared<SwapchainProvider>();

    mCoreProvider->linkHostApp(this);

    mSwapchainProvider->setCoreProvider(mCoreProvider.get());
    mCoreProvider->linkPresentationProvider(mSwapchainProvider.get());
    
    mSwapchainProvider->initGlfw();
    mCoreProvider->initVkInstance();
    mCoreProvider->initVkPhysicalDevice();
    mSwapchainProvider->initPresentationSurface();
    mCoreProvider->initVkLogicalDevice();
    mSwapchainProvider->initSwapchain();

    mSingleUniformBuffer.updateDevice(getPrimaryDeviceBundle());
}

void VulkanGraphicsApp::initCommandPool(){
    VkCommandPoolCreateInfo poolInfo;{
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = 0;
        poolInfo.queueFamilyIndex = *getPrimaryDeviceBundle().physicalDevice.mGraphicsIdx;
    }

    if(mCommandPool == VK_NULL_HANDLE){
        if(vkCreateCommandPool(getPrimaryDeviceBundle().logicalDevice.handle(), &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create command pool for graphics queue!");
        }
    }
}

void VulkanGraphicsApp::initRenderPipeline(){
    if(mVertexKey.empty()){
        throw std::runtime_error("Error! No vertex shader has been set! A vertex shader must be set using setVertexShader()!");
    }else if(mFragmentKey.empty()){
        throw std::runtime_error("Error! No fragment shader has been set! A vertex shader must be set using setFragmentShader()!");
    }

    vkutils::GraphicsPipelineConstructionSet& ctorSet =  mRenderPipeline.setupConstructionSet(VulkanDeviceHandlePair(getPrimaryDeviceBundle()), &mSwapchainProvider->getSwapchainBundle());
    
    mDepthBundle = ctorSet.mDepthBundle = vkutils::VulkanBasicRasterPipelineBuilder::autoCreateDepthBuffer(ctorSet);
    vkutils::VulkanBasicRasterPipelineBuilder::prepareFixedStages(ctorSet);

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
            fragShader = vkutils::load_shader_module(getPrimaryDeviceBundle().logicalDevice.handle(), STRIFY(SHADER_DIR) "/fallback.frag.spv");
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

    ctorSet.mVtxInputInfo.vertexBindingDescriptionCount = 1U;
    ctorSet.mVtxInputInfo.pVertexBindingDescriptions = &sObjVertexInput.getBindingDescription();
    ctorSet.mVtxInputInfo.vertexAttributeDescriptionCount = sObjVertexInput.getAttributeDescriptions().size();
    ctorSet.mVtxInputInfo.pVertexAttributeDescriptions = sObjVertexInput.getAttributeDescriptions().data();

    ctorSet.mPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ctorSet.mPipelineLayoutInfo.pNext = 0;
    ctorSet.mPipelineLayoutInfo.flags = 0;
    ctorSet.mPipelineLayoutInfo.setLayoutCount = 1;
    ctorSet.mPipelineLayoutInfo.pSetLayouts = &mUniformDescriptorSetLayout;
    ctorSet.mPipelineLayoutInfo.pushConstantRangeCount = 0;
    ctorSet.mPipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkutils::VulkanBasicRasterPipelineBuilder::prepareViewport(ctorSet);
    vkutils::VulkanBasicRasterPipelineBuilder::prepareRenderPass(ctorSet);

    mRenderPipeline.build(ctorSet);
}

void VulkanGraphicsApp::initCommands(){
    mCommandBuffers.resize(mSwapchainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo;{
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.commandBufferCount = mCommandBuffers.size();
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    // Allocate a command buffer for each in-flight frame extra for transfer operations. 
    if(vkAllocateCommandBuffers(getPrimaryDeviceBundle().logicalDevice.handle(), &allocInfo, mCommandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for(size_t i = 0; i < mCommandBuffers.size(); ++i){
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0 , nullptr};
        if(vkBeginCommandBuffer(mCommandBuffers[i], &beginInfo) != VK_SUCCESS){
            throw std::runtime_error("Failed to begin command recording!");
        }

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderBegin;{
            renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderBegin.pNext = nullptr;
            renderBegin.renderPass = mRenderPipeline.getRenderpass();
            renderBegin.framebuffer = mSwapchainFramebuffers[i];
            renderBegin.renderArea = {{0,0}, mSwapchainProvider->getSwapchainBundle().extent};
            renderBegin.clearValueCount = clearValues.size();
            renderBegin.pClearValues = clearValues.data();
        }

        vkCmdBeginRenderPass(mCommandBuffers[i], &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipeline.handle());
        
        for(size_t objIdx = 0; objIdx < mMultiShapeObjects.size(); ++objIdx){

            // Bind vertex buffer for object
            vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1U, &mMultiShapeObjects[objIdx].getVertexBuffer(), std::array<VkDeviceSize, 1>{0}.data());

            // Bind uniforms to graphics pipeline if they exist with correct dynamic offset for this object instance
            if(mMultiUniformBuffer->boundLayoutCount() > 0 || mSingleUniformBuffer.boundInterfaceCount() > 0){
                vkCmdBindDescriptorSets(
                    mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipeline.getLayout(),
                    0, 1, &mUniformDescriptorSets[i], 1, &mMultiUniformBuffer->getDynamicOffsets()[objIdx]
                );
            }

            // Bind index buffer for each shape and issue draw command. 
            for(size_t shapeIdx = 0; shapeIdx < mMultiShapeObjects[objIdx].shapeCount(); ++shapeIdx){
                vkCmdBindIndexBuffer(mCommandBuffers[i], mMultiShapeObjects[objIdx].getIndexBuffer(), mMultiShapeObjects[objIdx].getShapeOffset(shapeIdx), VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(mCommandBuffers[i], mMultiShapeObjects[objIdx].getShapeRange(shapeIdx), 1, 0U, 0U, 0U);
            }
            
        }

        vkCmdEndRenderPass(mCommandBuffers[i]);

        if(vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to end command buffer " + std::to_string(i));
        }
    }
}

void VulkanGraphicsApp::initFramebuffers(){
    mSwapchainFramebuffers.resize(mSwapchainProvider->getSwapchainBundle().views.size());
    for(size_t i = 0; i < mSwapchainProvider->getSwapchainBundle().views.size(); ++i){
        std::array<VkImageView, 2> attachmentViews = {mSwapchainProvider->getSwapchainBundle().views[i], mDepthBundle.depthImageView};
        VkFramebufferCreateInfo framebufferInfo;{
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.pNext = nullptr;
            framebufferInfo.flags = 0;
            framebufferInfo.renderPass = mRenderPipeline.getRenderpass();
            framebufferInfo.attachmentCount = attachmentViews.size();
            framebufferInfo.pAttachments = attachmentViews.data();
            framebufferInfo.width = mSwapchainProvider->getSwapchainBundle().extent.width;
            framebufferInfo.height = mSwapchainProvider->getSwapchainBundle().extent.height;
            framebufferInfo.layers = 1;
        }

        if(vkCreateFramebuffer(getPrimaryDeviceBundle().logicalDevice.handle(), &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS){
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
        failure |= vkCreateSemaphore(getPrimaryDeviceBundle().logicalDevice.handle(), &semaphoreCreate, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateSemaphore(getPrimaryDeviceBundle().logicalDevice.handle(), &semaphoreCreate, nullptr, &mRenderFinishSemaphores[i]) != VK_SUCCESS;
        failure |= vkCreateFence(getPrimaryDeviceBundle().logicalDevice.handle(), &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS;
    }
    if(failure){
        throw std::runtime_error("Failed to create semaphores!");
    }

}

void VulkanGraphicsApp::cleanupSwapchainDependents(){
    vkDestroyDescriptorPool(getPrimaryDeviceBundle().logicalDevice, mResourceDescriptorPool, nullptr);

    for(size_t i = 0; i < IN_FLIGHT_FRAME_LIMIT; ++i){
        vkDestroySemaphore(getPrimaryDeviceBundle().logicalDevice.handle(), mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(getPrimaryDeviceBundle().logicalDevice.handle(), mRenderFinishSemaphores[i], nullptr);
        vkDestroyFence(getPrimaryDeviceBundle().logicalDevice.handle(), mInFlightFences[i], nullptr);
    }

    vkFreeCommandBuffers(getPrimaryDeviceBundle().logicalDevice.handle(), mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());
    
    for(const VkFramebuffer& fb : mSwapchainFramebuffers){
        vkDestroyFramebuffer(getPrimaryDeviceBundle().logicalDevice.handle(), fb, nullptr);
    }

    vkDestroyImageView(getPrimaryDeviceBundle().logicalDevice, mDepthBundle.depthImageView, nullptr);
    vmaDestroyImage(VmaHost::getAllocator(getPrimaryDeviceBundle()), mDepthBundle.depthImage, mDepthBundle.mAllocation);

    mRenderPipeline.destroy();
}

void VulkanGraphicsApp::initTransferCmdBuffer(){
    VkCommandBufferAllocateInfo allocInfo;{
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    if(vkAllocateCommandBuffers(getPrimaryDeviceBundle().logicalDevice, &allocInfo, &mTransferCmdBuffer) != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate transfer command buffer!");
    }
}

void VulkanGraphicsApp::transferGeometry(){
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    assert(vkBeginCommandBuffer(mTransferCmdBuffer, &beginInfo) == VK_SUCCESS);
    for(ObjMultiShapeGeometry& geo : mMultiShapeObjects){
        if(geo.awaitingUploadTransfer()){
            geo.recordUploadTransferCommand(mTransferCmdBuffer);
        }
    }
    assert(vkEndCommandBuffer(mTransferCmdBuffer) == VK_SUCCESS);

    VkQueue transferQueue = getPrimaryDeviceBundle().logicalDevice.getTransferQueue();
    assert(transferQueue == getPrimaryDeviceBundle().logicalDevice.getGraphicsQueue());

    VkSubmitInfo submitInfo = vkutils::sSingleSubmitTemplate;
    submitInfo.pCommandBuffers = &mTransferCmdBuffer;
    if(vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS){
        throw std::runtime_error("Failed to transfer geometry data to the GPU!");
    }
    vkQueueWaitIdle(transferQueue);

    for(ObjMultiShapeGeometry& geo : mMultiShapeObjects){
        geo.freeStagingBuffer();
    }
}

void VulkanGraphicsApp::cleanup(){
    for(ObjMultiShapeGeometry& obj : mMultiShapeObjects){
        obj.freeAndReset();
    }

    for(std::pair<const std::string, VkShaderModule>& module : mShaderModules){
        vkDestroyShaderModule(getPrimaryDeviceBundle().logicalDevice.handle(), module.second, nullptr);
    }

    cleanupSwapchainDependents();

    mMultiUniformBuffer->freeAndReset();
    mMultiUniformBuffer = nullptr;
    mUniformDescriptorSets.clear();

    mSingleUniformBuffer.freeAndReset();

    if(mUniformDescriptorSetLayout != VK_NULL_HANDLE){
        vkDestroyDescriptorSetLayout(getPrimaryDeviceBundle().logicalDevice, mUniformDescriptorSetLayout, nullptr);
    }

    vkDestroyCommandPool(getPrimaryDeviceBundle().logicalDevice.handle(), mCommandPool, nullptr);

    mSwapchainProvider->cleanup();
    mCoreProvider->cleanup();
}

void VulkanGraphicsApp::initUniformResources() {
    if(mMultiUniformBuffer == nullptr){
        throw std::runtime_error("initMultiShapeUniformBuffer() must be called before initUniformResources()");
    }

    if(!mSingleUniformBuffer.getCurrentDevice().isValid()){
        throw std::runtime_error("initUniformResources called before mSingleUniformBuffer was given a valid device!");
    }

    mTotalUniformDescriptorSetCount = mSwapchainProvider->getSwapchainBundle().images.size();

    // Create layout from merged set of bindings from both the multi instance and single instance buffers
    const std::vector<VkDescriptorSetLayoutBinding>& multiBindings = mMultiUniformBuffer->getDescriptorSetLayoutBindings();
    const std::vector<VkDescriptorSetLayoutBinding>& singleBindings = mSingleUniformBuffer.getDescriptorSetLayoutBindings();
    std::vector<VkDescriptorSetLayoutBinding> mergedBindings;
    mergedBindings.reserve(multiBindings.size() + singleBindings.size());
    mergedBindings.insert(mergedBindings.end(), multiBindings.begin(), multiBindings.end());
    mergedBindings.insert(mergedBindings.end(), singleBindings.begin(), singleBindings.end());

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    {
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr;
        layoutInfo.flags = 0;
        layoutInfo.bindingCount = mergedBindings.size();
        layoutInfo.pBindings = mergedBindings.data();
    }

    if(vkCreateDescriptorSetLayout(getPrimaryDeviceBundle().logicalDevice, &layoutInfo, nullptr, &mUniformDescriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("Failed to create descriptor set layout for graphics app uniforms!");
    }

    initUniformDescriptorPool();
    allocateDescriptorSets();
    writeDescriptorSets();
}
    
void VulkanGraphicsApp::initUniformDescriptorPool() {
    uint32_t dynamicPoolSize = mTotalUniformDescriptorSetCount*mMultiUniformBuffer->boundLayoutCount();
    uint32_t staticPoolSize = mTotalUniformDescriptorSetCount*mSingleUniformBuffer.boundInterfaceCount();
    VkDescriptorPoolSize poolSizes[2] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dynamicPoolSize},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, staticPoolSize}
    };

    VkDescriptorPoolCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.maxSets = static_cast<uint32_t>(mTotalUniformDescriptorSetCount);
        createInfo.poolSizeCount = 2;
        createInfo.pPoolSizes = poolSizes;
    }

    if(vkCreateDescriptorPool(getPrimaryDeviceBundle().logicalDevice, &createInfo, nullptr, &mResourceDescriptorPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to create uniform descriptor pool");
    }
}
    
void VulkanGraphicsApp::allocateDescriptorSets() {
    assert(mTotalUniformDescriptorSetCount != 0);

    std::vector<VkDescriptorSetLayout> layouts(mTotalUniformDescriptorSetCount, mUniformDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo;
    {
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = mResourceDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(mTotalUniformDescriptorSetCount);
        allocInfo.pSetLayouts = layouts.data();
    }
    
    mUniformDescriptorSets.resize(mTotalUniformDescriptorSetCount, VK_NULL_HANDLE);
    VkResult allocResult = vkAllocateDescriptorSets(getPrimaryDeviceBundle().logicalDevice, &allocInfo, mUniformDescriptorSets.data());
    if(allocResult != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate uniform descriptor sets");
    }
}

void VulkanGraphicsApp::writeDescriptorSets(){
    std::map<uint32_t, VkDescriptorBufferInfo> bufferInfos = merge(mSingleUniformBuffer.getDescriptorBufferInfos(), mMultiUniformBuffer->getDescriptorBufferInfos());

    std::vector<VkWriteDescriptorSet> setWriters;
    setWriters.reserve(mUniformDescriptorSets.size() * bufferInfos.size());

    VkBuffer staticUB = mSingleUniformBuffer.handle();

    for(VkDescriptorSet descriptorSet : mUniformDescriptorSets){
        for(const auto& info : bufferInfos){
            setWriters.emplace_back(
                VkWriteDescriptorSet{
                    /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    /* pNext = */ nullptr, 
                    /* dstSet = */ descriptorSet,
                    /* dstBinding = */ info.first,
                    /* dstArrayElement = */ 0,
                    /* descriptorCount = */ 1,
                    /* descriptorType = */ info.second.buffer == staticUB ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    /* pImageInfo = */ nullptr,
                    /* pBufferInfo = */ &info.second,
                    /* pTexelBufferView = */ nullptr
                }
            );
        }
    }
    vkUpdateDescriptorSets(getPrimaryDeviceBundle().logicalDevice, setWriters.size(), setWriters.data(), 0, nullptr);
}

void VulkanGraphicsApp::reinitUniformResources() {
    mSingleUniformBuffer.updateDevice();
    mMultiUniformBuffer->updateDevice();
    writeDescriptorSets();
}