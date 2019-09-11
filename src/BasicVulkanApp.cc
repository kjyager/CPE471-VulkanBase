#include "BasicVulkanApp.h"
#include <iostream>
#include <cassert>

static void error_callback(int error, const char* description){std::cerr << "glfw error: " << description << std::endl;};
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(window, true);
	}
};

static VkPhysicalDevice select_physical_device(const std::vector<VkPhysicalDevice>& aDevices);


void BasicVulkanApp::init(){
    initGlfw();
    initVulkan();
}

const VkApplicationInfo& BasicVulkanApp::getAppInfo() const{
    const static VkApplicationInfo sAppInfo = {
        /* appInfo.sType = */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* appInfo.pNext = */ nullptr,
        /* appInfo.pApplicationName = */ "Vulkan Application",
        /* appInfo.applicationVersion = */ VK_MAKE_VERSION(0, 0, 0),
        /* appInfo.pEngineName = */ "Tutorial",
        /* appInfo.engineVersion = */ VK_MAKE_VERSION(0,0,0),
        /* appInfo.apiVersion = */ VK_API_VERSION_1_1
    };
    return(sAppInfo);
}
const std::vector<std::string>& BasicVulkanApp::getRequiredValidationLayers() const {
    const static std::vector<std::string> sRequired = {
        // None
    };
    return(sRequired);
}
const std::vector<std::string>& BasicVulkanApp::getRequestedValidationLayers() const {
    const static std::vector<std::string> sRequested = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker"
    };
    return(sRequested);
}
const std::vector<std::string>& BasicVulkanApp::getRequiredInstanceExtensions() const {
    const static std::vector<std::string> sRequired = {
        // None
    };
    return(sRequired);
}
const std::vector<std::string>& BasicVulkanApp::getRequestedInstanceExtensions() const {
    const static std::vector<std::string> sRequested = {
        // None
    };
    return(sRequested);
}
const std::vector<std::string>& BasicVulkanApp::getRequiredDeviceExtensions() const {
    const static std::vector<std::string> sRequired = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    return(sRequired);
}
const std::vector<std::string>& BasicVulkanApp::getRequestedDeviceExtensions() const {
    const static std::vector<std::string> sRequested = {
        // None
    };
    return(sRequested);
}
const std::unordered_map<std::string, bool>& BasicVulkanApp::getValidationLayersState() const {
    return(_mValidationLayers);
}
const std::unordered_map<std::string, bool>& BasicVulkanApp::getExtensionState() const {
    return(_mExtensions);
}

void BasicVulkanApp::initGlfw(){
	glfwSetErrorCallback(error_callback);

	if(!glfwInit()) {
		throw std::runtime_error("Unable to initialize glfw for Vulkan App");
		exit(0);
	}

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, true);
	glfwWindowHint(GLFW_RESIZABLE, false);

	GLFWmonitor* monitor = nullptr; // autoDetectScreen(&config.w_width, &config.w_height);
	// config.w_width = 1280; config.w_height = 720;

	mWindow = glfwCreateWindow(mViewportWidth, mViewportHeight, "Vulkan App", monitor, nullptr);
	if(!mWindow) {
		glfwTerminate();
		exit(0);
	}

	glfwSetKeyCallback(mWindow, key_callback);
}

std::vector<std::string> BasicVulkanApp::gatherExtensionInfo(){
    // Create list of extension names that will be returned
    std::vector<std::string> extensionList;

    // Get the list of extensions required by GLFW
    uint32_t glfw_ext_count = 0;
    const char** glfw_req_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    printf("GLFW requires the following extensions:\n");
    for(uint32_t i = 0; i < glfw_ext_count; ++i){
        printf("  - %s\n", glfw_req_exts[i]);
    }

    // Get the required and requested extensions for this application
    const std::vector<std::string>& required = getRequiredInstanceExtensions();
    const std::vector<std::string>& requested = getRequestedInstanceExtensions();

    //reserve space in the extension list, and insert the GLFW required extensions.
    extensionList.reserve(glfw_ext_count + required.size() + requested.size());
    extensionList.insert(extensionList.end(), glfw_req_exts, glfw_req_exts + glfw_ext_count);

    // Enumerate all available instance extentions
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, instanceExtensions.data());

    //Match up requested and required with their counterparts in the list of all available extensions.
    vkutils::find_extension_matches(instanceExtensions, required, requested, extensionList, &_mExtensions);


    return(extensionList);
}

std::vector<std::string> BasicVulkanApp::gatherValidationLayers(){
    // Create list of extension names that will be returned
    std::vector<std::string> layerList;

    // Get the required and requested validation layers for this application
    const std::vector<std::string>& required = getRequiredValidationLayers();
    const std::vector<std::string>& requested = getRequestedValidationLayers();

    //reserve space in the layer list
    layerList.reserve(required.size() + requested.size());

    // Enumerate all available validation layers
    uint32_t propCount = 0;
    vkEnumerateInstanceLayerProperties(&propCount, nullptr);
    std::vector<VkLayerProperties> instanceLayers(propCount);
    vkEnumerateInstanceLayerProperties(&propCount, instanceLayers.data());

    //Match up requested and required with their counterparts in the list of all available extensions.
    vkutils::find_layer_matches(instanceLayers, required, requested, layerList, &_mValidationLayers);

    return(layerList);
}

void BasicVulkanApp::initVulkan(){
   createVkInstance();
   initPresentationSurface();
   initVkDevices();
}

void BasicVulkanApp::createVkInstance(){

    std::vector<std::string> extensionsList = gatherExtensionInfo();
    std::vector<std::string> validationLayersList = gatherValidationLayers();
    std::vector<const char*> extensionListCstr = vkutils::strings_to_cstrs(extensionsList);
    std::vector<const char*> validationLayersListCstr = vkutils::strings_to_cstrs(validationLayersList);
    
    VkInstanceCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &getAppInfo();
        createInfo.enabledExtensionCount = extensionsList.size();
        createInfo.ppEnabledExtensionNames = extensionListCstr.data();
        createInfo.enabledLayerCount = validationLayersList.size();
        createInfo.ppEnabledLayerNames = validationLayersListCstr.data();
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &mVkInstance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void BasicVulkanApp::initVkDevices(){
    // Enumerate the list of visible Vulkan supporting physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, devices.data());

    if(deviceCount == 0){
        throw std::runtime_error("No Vulkan supporting devices found!");
    }

    // Select a device to use based on some the type
    VkPhysicalDevice selectedDevice = select_physical_device(devices);
    if(selectedDevice == VK_NULL_HANDLE){
        throw std::runtime_error("No compatible device available!");
    }

    // Wrap physical device with utility class. 
    mPhysDevice = VulkanPhysicalDevice(selectedDevice); 
    assert(mPhysDevice.coreFeaturesIdx && mPhysDevice.getPresentableQueueIndex(mVkSurface));

    const std::vector<std::string>& requiredExts = getRequiredDeviceExtensions();
    const std::vector<std::string>& requestedExts = getRequestedDeviceExtensions();

    std::vector<std::string> deviceExtensions;

    vkutils::find_extension_matches(mPhysDevice.mAvailableExtensions, requiredExts, requestedExts, deviceExtensions);

    mLogicalDevice = mPhysDevice.createPresentableCoreDevice(mVkSurface, vkutils::strings_to_cstrs(deviceExtensions));
}

void BasicVulkanApp::initPresentationSurface(){
    if(glfwCreateWindowSurface(mVkInstance, mWindow, nullptr, &mVkSurface) != VK_SUCCESS){
        throw std::runtime_error("Unable to create presentable surface on GLFW window!");
    }
}

void BasicVulkanApp::cleanup(){
    vkDestroySurfaceKHR(mVkInstance, mVkSurface, nullptr);
    vkDestroyDevice(mLogicalDevice.handle(), nullptr);
    vkDestroyInstance(mVkInstance, nullptr);
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

static bool confirm_queue_fam(VkPhysicalDevice aDevice, uint32_t aBitmask){
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueCount, queueProperties.data());

    uint32_t maskResult = 0;
    for(const VkQueueFamilyProperties& queueFamily : queueProperties){
        if(queueFamily.queueCount > 0)
            maskResult = maskResult | (aBitmask & queueFamily.queueFlags);
    }

    return(maskResult == aBitmask);
}

static int score_physical_device(VkPhysicalDevice aDevice){
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(aDevice, &properties);
    vkGetPhysicalDeviceFeatures(aDevice, &features);

    int score = 0;

    switch(properties.deviceType){
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            score += 0000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 2000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 4000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 3000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 1000;
            break;
    }

    score = confirm_queue_fam(aDevice, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) ? score : -1;

    //TODO: More metrics

    return(score);
}

static VkPhysicalDevice select_physical_device(const std::vector<VkPhysicalDevice>& aDevices){
    int high_score = -1;
    size_t max_index = 0;
    std::vector<int> scores(aDevices.size());
    for(size_t i = 0; i < aDevices.size(); ++i){
        int score = score_physical_device(aDevices[i]);
        if(score > high_score){
            high_score = score;
            max_index = i;
        }
    }
    if(high_score >= 0){
        return(aDevices[max_index]);
    }else{
        return(VK_NULL_HANDLE);
    }
    
}