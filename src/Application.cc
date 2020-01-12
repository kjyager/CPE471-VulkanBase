#include "utils/common.h"
#include "Application.h"
#include "data/VertexInput.h"
#include <glm/glm.hpp>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>


struct SimpleVertex{
    glm::vec3 pos;
    glm::vec4 color;
};

    
void Application::init(){
    BasicVulkanApp::init();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();
}

void Application::run(){
    const static size_t timeBufferSize = 1024;
    std::chrono::high_resolution_clock::duration renderSum(0);
    std::chrono::high_resolution_clock::duration localRenderSum(0);

    while(!glfwWindowShouldClose(mWindow)){
        glfwPollEvents();

        const WindowFlags& windowFlags = sWindowFlags[mWindow];
        bool windowVisible = glfwGetWindowAttrib(mWindow, GLFW_VISIBLE) == GLFW_TRUE;
        if(windowFlags.iconified || !windowVisible){
            // Give some cycles back to the OS :)
            std::this_thread::sleep_for(std::chrono::milliseconds(112));
            continue;
        }

        auto start = std::chrono::high_resolution_clock::now();
        render();
        auto finish = std::chrono::high_resolution_clock::now();

        renderSum += finish-start;
        localRenderSum += finish-start;
        if(mFrameNumber % timeBufferSize == 0 && mFrameNumber != 0){
            uint64_t totalMicro = std::chrono::duration_cast<std::chrono::microseconds>(localRenderSum).count();
            double averageFrametimeMicro = static_cast<double>(totalMicro) / static_cast<double>(timeBufferSize);
            printf("%.02f fps (%g microseconds)\n", 1e6/averageFrametimeMicro, averageFrametimeMicro);
            localRenderSum = std::chrono::high_resolution_clock::duration(0);
        }
        ++mFrameNumber;
    }

    uint64_t totalMicro = std::chrono::duration_cast<std::chrono::microseconds>(renderSum).count();
    double averageFrametimeMicro = static_cast<double>(totalMicro) / static_cast<double>(mFrameNumber);
    printf("Average Peformance: %.02f fps (%g ms)\n", 1e6/averageFrametimeMicro, averageFrametimeMicro/1e3);
    
    vkDeviceWaitIdle(mLogicalDevice.handle());
}

void Application::resetRenderSetup(){
    vkDeviceWaitIdle(mLogicalDevice.handle());

    cleanupSwapchainDependents();
    BasicVulkanApp::cleanupSwapchain();

    BasicVulkanApp::initSwapchain();
    initRenderPipeline();
    initFramebuffers();
    initCommands();
    initSync();

    sWindowFlags[mWindow].resized = false;
}

void Application::render(){
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

void Application::cleanup(){
    for(std::pair<const std::string, VkShaderModule>& module : mShaderModules){
        vkDestroyShaderModule(mLogicalDevice.handle(), module.second, nullptr);
    }
    Application::cleanupSwapchainDependents();
    vkDestroyCommandPool(mLogicalDevice.handle(), mCommandPool, nullptr);
    BasicVulkanApp::cleanup();
}

void Application::cleanupSwapchainDependents(){
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


void Application::initRenderPipeline(){
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


    using SimpleVertexInput = VertexInputTemplate<SimpleVertex>;

    static SimpleVertexInput sSimpleVertexInput( /*binding = */ 0U,
        /*vertex attribute descriptions = */ {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SimpleVertex, pos)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SimpleVertex, color)}
        }
    );
    ctorSet.mVtxInputInfo.pVertexBindingDescriptions = &sSimpleVertexInput.getBindingDescription();
    ctorSet.mVtxInputInfo.vertexBindingDescriptionCount = 1U;
    ctorSet.mVtxInputInfo.pVertexAttributeDescriptions = sSimpleVertexInput.getAttributeDescriptions().data();
    ctorSet.mVtxInputInfo.vertexAttributeDescriptionCount = sSimpleVertexInput.getAttributeDescriptions().size();

    vkutils::BasicVulkanRenderPipeline::prepareViewport(ctorSet);
    vkutils::BasicVulkanRenderPipeline::prepareRenderPass(ctorSet);
    mRenderPipeline.build(ctorSet);
}

void Application::initCommands(){
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

        using SimpleVertexBuffer = VertexAttributeBuffer<SimpleVertex>;

        static std::shared_ptr<SimpleVertexBuffer> triangle(new SimpleVertexBuffer(
            {
                {glm::vec3(-0.5, 0.5, 0.0), glm::vec4(1.0, 0.0, 0.0, 1.0)},
                {glm::vec3(0.0, -.5, 0.0), glm::vec4(0.0, 1.0, 0.0, 1.0)},
                {glm::vec3(0.5, 0.5, 0.0), glm::vec4(0.0, 0.0, 1.0, 1.0)},
            },
            {mLogicalDevice.handle(), mPhysDevice.handle()}
        ));

        assert(triangle->getDeviceSyncState() == SimpleVertexBuffer::DEVICE_IN_SYNC);

        vkCmdBeginRenderPass(mCommandBuffers[i], &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderPipeline.getPipeline());
        vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1, &triangle->handle(), std::array<VkDeviceSize, 1>{0}.data());
        vkCmdDraw(mCommandBuffers[i], triangle->vertexCount(), 1, 0, 0);
        vkCmdEndRenderPass(mCommandBuffers[i]);

        if(vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to end command buffer " + std::to_string(i));
        }
    }
}

void Application::initFramebuffers(){
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


void Application::initSync(){
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
