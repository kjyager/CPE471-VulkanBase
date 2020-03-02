#include "SwapchainProvider.h"
#include "utils/common.h"
#include <iostream>

#define NVIDIA_VENDOR_ID 0x10DE

static void error_callback(int error, const char* description){std::cerr << "glfw error: " << description << std::endl;} 
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(window, true);
	}
}

std::unordered_map<GLFWwindow*, SwapchainProvider::WindowFlags> SwapchainProvider::sWindowFlags;

void SwapchainProvider::initGlfw(){
	glfwSetErrorCallback(error_callback);

	if(!glfwInit()) {
		throw std::runtime_error("Unable to initialize glfw for Vulkan App");
		exit(0);
	}

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, true);
	glfwWindowHint(GLFW_RESIZABLE, true);

	GLFWmonitor* monitor = nullptr; // autoDetectScreen(&config.w_width, &config.w_height);
	// config.w_width = 1280; config.w_height = 720;

	mWindow = glfwCreateWindow(mViewportExtent.width, mViewportExtent.height, "Vulkan App", monitor, nullptr);
	if(!mWindow) {
		glfwTerminate();
		exit(0);
	}

    sWindowFlags.emplace(std::pair<GLFWwindow*, SwapchainProvider::WindowFlags>(mWindow, {}));

	glfwSetKeyCallback(mWindow, key_callback);

    GLFWframebuffersizefun resizeCallback = [](GLFWwindow* aWindow, int, int) {
        SwapchainProvider::sWindowFlags[aWindow].resized = true;
    };
    glfwSetFramebufferSizeCallback(mWindow, resizeCallback);

    GLFWwindowiconifyfun iconifyCallback = [](GLFWwindow* aWindow, int aIconified){
        SwapchainProvider::sWindowFlags[aWindow].iconified = (aIconified == GLFW_TRUE) ? true : false;
    };
    glfwSetWindowIconifyCallback(mWindow, iconifyCallback);

    GLFWwindowfocusfun focusCallback = [](GLFWwindow* aWindow, int aFocus){
        SwapchainProvider::sWindowFlags[aWindow].focus = (aFocus == GLFW_TRUE) ? true : false;
    };
    glfwSetWindowFocusCallback(mWindow, focusCallback);
}

const std::vector<std::string>& SwapchainProvider::getRequiredInstanceExtensions() const {
    uint32_t glfwExtCount = 0;
    const char** glfwReqExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    static std::vector<std::string> sRequired(glfwReqExts, glfwReqExts + glfwExtCount);

    return(sRequired);
}

const std::vector<std::string>& SwapchainProvider::getRequiredDeviceExtensions() const {
    const static std::vector<std::string> sRequired = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    return(sRequired);
}

void SwapchainProvider::initPresentationSurface(){
    if(glfwCreateWindowSurface(getVulkanInstance(), mWindow, nullptr, &mVkSurface) != VK_SUCCESS){
        throw std::runtime_error("Unable to create presentable surface on GLFW window!");
    }
}

void SwapchainProvider::initSwapchain(){
    SwapChainSupportInfo chainInfo = getPrimaryDeviceBundle().physicalDevice.getSwapChainSupportInfo(mVkSurface);
    if(chainInfo.formats.empty() || chainInfo.presentation_modes.empty()){
        throw std::runtime_error("The selected physical device does not support presentation!");
    }

    mSwapchainBundle.surface_format = selectSurfaceFormat(chainInfo.formats);

    mSwapchainBundle.presentation_mode = selectPresentationMode(chainInfo.presentation_modes);
    
#ifdef __unix__
    // The bug was not fixed :|
    if(getPrimaryDeviceBundle().physicalDevice.mProperties.vendorID == NVIDIA_VENDOR_ID){
        // Nvidia has a nasty bug on systems using Nvidia prime sync that causes FIFO present modes 
        // to freeze the application and the display in general. For now just fallback to immediate mode.
        fprintf(stderr, "Warning: Nvidia device detected. Forcing use of immediate present mode.\n");
        mSwapchainBundle.presentation_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
#endif 


    mViewportExtent = selectSwapChainExtent(chainInfo.capabilities);
    mSwapchainBundle.extent = mViewportExtent;

    if(chainInfo.capabilities.maxImageCount == 0){
        mSwapchainBundle.requested_image_count = chainInfo.capabilities.minImageCount + 1;
    }else{
        mSwapchainBundle.requested_image_count = std::min(chainInfo.capabilities.minImageCount + 1, chainInfo.capabilities.maxImageCount);
    }

    std::vector<uint32_t> queueFamilyIndices;

    uint32_t presFamilyIdx, gfxFamilyIdx;
    presFamilyIdx = gfxFamilyIdx = std::numeric_limits<uint32_t>::max();

    if(getPrimaryDeviceBundle().physicalDevice.coreFeaturesIdx){
        presFamilyIdx = gfxFamilyIdx = getPrimaryDeviceBundle().physicalDevice.coreFeaturesIdx.value();
    }else if(getPrimaryDeviceBundle().physicalDevice.mGraphicsIdx.value() == getPrimaryDeviceBundle().physicalDevice.getPresentableQueueIndex(mVkSurface).value()){
        presFamilyIdx = gfxFamilyIdx = getPrimaryDeviceBundle().physicalDevice.mGraphicsIdx.value();
    }

    // We only need to mark the family indices if they are different. 
    if(presFamilyIdx != gfxFamilyIdx){
        queueFamilyIndices.emplace_back(*getPrimaryDeviceBundle().physicalDevice.mGraphicsIdx);
        queueFamilyIndices.emplace_back(*getPrimaryDeviceBundle().physicalDevice.getPresentableQueueIndex(mVkSurface));
    }
    
    VkSwapchainCreateInfoKHR createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.surface = mVkSurface;
        createInfo.minImageCount = mSwapchainBundle.requested_image_count;
        createInfo.imageFormat = mSwapchainBundle.surface_format.format;
        createInfo.imageColorSpace = mSwapchainBundle.surface_format.colorSpace;
        createInfo.imageExtent = mSwapchainBundle.extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = queueFamilyIndices.empty() ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        createInfo.preTransform = chainInfo.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = mSwapchainBundle.presentation_mode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
    }

    if(vkCreateSwapchainKHR(getPrimaryDeviceBundle().logicalDevice.handle(), &createInfo, nullptr, &mSwapchainBundle.swapchain) != VK_SUCCESS){
        throw std::runtime_error("Unable to create swapchain!");
    }

    vkGetSwapchainImagesKHR(getPrimaryDeviceBundle().logicalDevice.handle(), mSwapchainBundle.swapchain, &mSwapchainBundle.image_count, nullptr);
    mSwapchainBundle.images.resize(mSwapchainBundle.image_count);
    vkGetSwapchainImagesKHR(getPrimaryDeviceBundle().logicalDevice.handle(), mSwapchainBundle.swapchain, &mSwapchainBundle.image_count, mSwapchainBundle.images.data());

    initSwapchainViews();
}

void SwapchainProvider::initSwapchainViews(){
    mSwapchainBundle.views.resize(mSwapchainBundle.image_count);
    for(size_t i = 0; i < mSwapchainBundle.image_count; ++i){
        VkImageViewCreateInfo createInfo;
        {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.image = mSwapchainBundle.images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = mSwapchainBundle.surface_format.format;
            createInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            createInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        }
        if(vkCreateImageView(getPrimaryDeviceBundle().logicalDevice.handle(), &createInfo, nullptr, &mSwapchainBundle.views[i]) != VK_SUCCESS){
            throw std::runtime_error("Failed to create image view for swap chain image " + std::to_string(i));
        }
    }
}

const VkSurfaceFormatKHR SwapchainProvider::selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& aFormats) const{
    for(const VkSurfaceFormatKHR& format: aFormats){
        if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return(format);
        }
    }
    return(aFormats[0]);
}

const VkPresentModeKHR SwapchainProvider::selectPresentationMode(const std::vector<VkPresentModeKHR>& aModes) const{
    const static std::unordered_map<VkPresentModeKHR, int> valueMap = {
        {VK_PRESENT_MODE_IMMEDIATE_KHR, 1},
        {VK_PRESENT_MODE_MAILBOX_KHR, 6},
        {VK_PRESENT_MODE_FIFO_KHR, 3},
        {VK_PRESENT_MODE_FIFO_RELAXED_KHR, 2},
        {VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, 0},
        {VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR, 0}
    };

    int highScore = -1;
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    for(const VkPresentModeKHR& mode : aModes){
        int score = valueMap.at(mode);
        if(score > highScore){
            highScore = score;
            bestMode = mode;
        }
    }
    return(bestMode);
}

const VkExtent2D SwapchainProvider::selectSwapChainExtent(const VkSurfaceCapabilitiesKHR& aCapabilities) const{
    if(aCapabilities.currentExtent.width != 0xFFFFFFFF){
        return(aCapabilities.currentExtent);
    }else{
        VkExtent2D resultExtent;
        
        int x,y;
        glfwGetFramebufferSize(mWindow, &x, &y);
        if(x < 0 || y < 0) { x = 0; y = 0; }
        VkExtent2D glfwExtent = {static_cast<uint32_t>(x), static_cast<uint32_t>(y)}; 

        resultExtent.width = CLAMP(aCapabilities.maxImageExtent.width, aCapabilities.minImageExtent.width, glfwExtent.width);
        resultExtent.height = CLAMP(aCapabilities.maxImageExtent.height, aCapabilities.minImageExtent.height, glfwExtent.height);
        return(resultExtent);
    }
}

void SwapchainProvider::cleanupSwapchain(){
    for(const VkImageView& view : mSwapchainBundle.views){
        vkDestroyImageView(getPrimaryDeviceBundle().logicalDevice.handle(), view, nullptr);
    }
    mSwapchainBundle.views.clear();

    if(mSwapchainBundle.swapchain != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(getPrimaryDeviceBundle().logicalDevice.handle(), mSwapchainBundle.swapchain, nullptr);
        mSwapchainBundle.swapchain = VK_NULL_HANDLE;
    }
}

void SwapchainProvider::cleanup(){
    cleanupSwapchain();

    if(mVkSurface != VK_NULL_HANDLE){
        vkDestroySurfaceKHR(getVulkanInstance(), mVkSurface, nullptr);
        mVkSurface = VK_NULL_HANDLE;
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}