#include "application/VulkanSetupCore.h"
#include "utils/common.h"
#include "vkutils/VmaHost.h"
#include "utils/BufferedTimer.h"
#include <iostream>
#include <algorithm>
#include <cassert>

// Uncomment to select device by command line.
// #define MANUAL_DEVICE_SELECTION

#ifdef MANUAL_DEVICE_SELECTION
#include <sstream>
static VkPhysicalDevice manual_select_physical_device(const std::vector<VkPhysicalDevice>& aDevices);
#endif

void VulkanSetupCore::registerDependentProvider(const VulkanProviderInterface* aProvider) {
    _mDependentProviders.emplace(aProvider);
}

void VulkanSetupCore::linkPresentationProvider(const PresentationProviderInterface* aProvider) {
    mPresentationProvider = aProvider;
    _mDependentProviders.emplace(aProvider);
}

std::vector<std::string> VulkanSetupCore::gatherInstanceExtensions(){
    // Create list of extension names that will be returned
    std::vector<std::string> extensionList;

    // Get the required and requested extensions for this application
    const std::vector<std::string>& selfRequired = getRequiredInstanceExtensions();
    const std::vector<std::string>& selfRequested = getRequestedInstanceExtensions();
    std::set<std::string> required(selfRequired.begin(), selfRequired.end());
    std::set<std::string> requested(selfRequested.begin(), selfRequested.end());
    for(const VulkanProviderInterface* dependent : _mDependentProviders){
        const std::vector<std::string>& dRequested = dependent->getRequestedInstanceExtensions();
        const std::vector<std::string>& dRequired = dependent->getRequiredInstanceExtensions();
        requested.insert(dRequested.begin(), dRequested.end());
        required.insert(dRequired.begin(), dRequired.end());
    }
    if(mHostApp != nullptr){
        const std::vector<std::string>& hRequested = mHostApp->getRequestedInstanceExtensions();
        const std::vector<std::string>& hRequired = mHostApp->getRequiredInstanceExtensions();
        requested.insert(hRequested.begin(), hRequested.end());
        required.insert(hRequired.begin(), hRequired.end());
    }

    // Enumerate all available instance extentions
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, instanceExtensions.data());

    extensionList.reserve(required.size() + requested.size());

    //Match up requested and required with their counterparts in the list of all available extensions.
    vkutils::find_extension_matches(instanceExtensions, required, requested, extensionList, &_mInstExtensions);

    return(extensionList);
}

std::vector<std::string> VulkanSetupCore::gatherDeviceExtensions(){
    // Create list of extension names that will be returned
    std::vector<std::string> extensionList;

    // Get the required and requested extensions for this application
    const std::vector<std::string>& selfRequired = getRequiredDeviceExtensions();
    const std::vector<std::string>& selfRequested = getRequestedDeviceExtensions();
    std::set<std::string> required(selfRequired.begin(), selfRequired.end());
    std::set<std::string> requested(selfRequested.begin(), selfRequested.end());
    for(const VulkanProviderInterface* dependent : _mDependentProviders){
        const std::vector<std::string>& dRequested = dependent->getRequestedDeviceExtensions();
        const std::vector<std::string>& dRequired = dependent->getRequiredDeviceExtensions();
        requested.insert(dRequested.begin(), dRequested.end());
        required.insert(dRequired.begin(), dRequired.end());
    }
    if(mHostApp != nullptr){
        const std::vector<std::string>& hRequested = mHostApp->getRequestedDeviceExtensions();
        const std::vector<std::string>& hRequired = mHostApp->getRequiredDeviceExtensions();
        requested.insert(hRequested.begin(), hRequested.end());
        required.insert(hRequired.begin(), hRequired.end());
    }

    // Enumerate all available instance extentions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(getPrimaryDeviceBundle().physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions(extCount);
    vkEnumerateDeviceExtensionProperties(getPrimaryDeviceBundle().physicalDevice, nullptr, &extCount, instanceExtensions.data());

    extensionList.reserve(required.size() + requested.size());

    //Match up requested and required with their counterparts in the list of all available extensions.
    vkutils::find_extension_matches(instanceExtensions, required, requested, extensionList, &_mInstExtensions);

    return(extensionList);
}

std::vector<std::string> VulkanSetupCore::gatherValidationLayers(){
    // Create list of extension names that will be returned
    std::vector<std::string> layerList;

    // Get the required and requested validation layers for this application
    const std::vector<std::string>& selfRequired = getRequiredValidationLayers();
    const std::vector<std::string>& selfRequested = getRequestedValidationLayers();
    std::set<std::string> required(selfRequired.begin(), selfRequired.end());
    std::set<std::string> requested(selfRequested.begin(), selfRequested.end());
    for(const VulkanProviderInterface* dependent : _mDependentProviders){
        const std::vector<std::string>& dRequested = dependent->getRequestedValidationLayers();
        const std::vector<std::string>& dRequired = dependent->getRequiredValidationLayers();
        required.insert(dRequired.begin(), dRequired.end());
        requested.insert(dRequested.begin(), dRequested.end());
    }
    if(mHostApp != nullptr){
        const std::vector<std::string>& hRequested = mHostApp->getRequestedValidationLayers();
        const std::vector<std::string>& hRequired = mHostApp->getRequiredValidationLayers();
        requested.insert(hRequested.begin(), hRequested.end());
        required.insert(hRequired.begin(), hRequired.end());
    }

    // Enumerate all available validation layers
    uint32_t propCount = 0;
    vkEnumerateInstanceLayerProperties(&propCount, nullptr);
    std::vector<VkLayerProperties> instanceLayers(propCount);
    vkEnumerateInstanceLayerProperties(&propCount, instanceLayers.data());

    // reserve space in the layer list
    layerList.reserve(required.size() + requested.size());

    //Match up requested and required with their counterparts in the list of all available extensions.
    vkutils::find_layer_matches(instanceLayers, required, requested, layerList, &_mValidationLayers);

    return(layerList);
}

VkQueueFlags VulkanSetupCore::getRequiredQueueFlags() const {
    VkQueueFlags flags = 0;
    for(const VulkanProviderInterface* dependent : _mDependentProviders){
        flags |= dependent->getRequiredQueueFlags();
    }
    return(flags);
}

void VulkanSetupCore::initVulkan(){
   initVkInstance();
   initVkPhysicalDevice();
   initVkLogicalDevice();
   VmaHost::getAllocator(VulkanDeviceHandlePair(mDeviceBundle));
}

void VulkanSetupCore::initVkInstance(){
    const static VkApplicationInfo sAppInfo = {
        /* appInfo.sType = */ VK_STRUCTURE_TYPE_APPLICATION_INFO,
        /* appInfo.pNext = */ nullptr,
        /* appInfo.pApplicationName = */ "Vulkan Application",
        /* appInfo.applicationVersion = */ VK_MAKE_VERSION(0, 0, 0),
        /* appInfo.pEngineName = */ "KJY",
        /* appInfo.engineVersion = */ VK_MAKE_VERSION(0,0,0),
        /* appInfo.apiVersion = */ VULKAN_BASE_VK_API_VERSION
    };

    initVkInstance(mHostApp != nullptr ? mHostApp->getAppInfo() : sAppInfo);
}

void VulkanSetupCore::initVkInstance(const VkApplicationInfo& aAppInfo){
    std::vector<std::string> extensionsList = gatherInstanceExtensions();
    std::vector<std::string> validationLayersList = gatherValidationLayers();
    std::vector<const char*> extensionListCstr = vkutils::strings_to_cstrs(extensionsList);
    std::vector<const char*> validationLayersListCstr = vkutils::strings_to_cstrs(validationLayersList);
    
    VkInstanceCreateInfo createInfo;
    {
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &aAppInfo;
        createInfo.enabledExtensionCount = extensionsList.size();
        createInfo.ppEnabledExtensionNames = extensionListCstr.data();
        createInfo.enabledLayerCount = validationLayersList.size();
        createInfo.ppEnabledLayerNames = validationLayersListCstr.data();
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &mVkInstance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed to create Vulkan instance");
    }
	VmaHost::setVkInstance(mVkInstance);
}

void VulkanSetupCore::initVkPhysicalDevice(){
    // Enumerate the list of visible Vulkan supporting physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, devices.data());

    if(deviceCount == 0){
        throw std::runtime_error("No Vulkan supporting devices found!");
    }

    // Select a device to use based on some the type
    #ifndef MANUAL_DEVICE_SELECTION
        VkPhysicalDevice selectedDevice = vkutils::select_physical_device(devices);
    #else
        VkPhysicalDevice selectedDevice = manual_select_physical_device(devices);
    #endif

    if(selectedDevice == VK_NULL_HANDLE){
        throw std::runtime_error("No compatible device available!");
    }
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(selectedDevice, &props);
    std::cout << "Selected physical device '" << props.deviceName << "'(" << std::hex << props.deviceID << ")" << std::dec << std::endl;    

    // Wrap physical device with utility class. 
    mDeviceBundle.physicalDevice = VulkanPhysicalDevice(selectedDevice);
}

void VulkanSetupCore::initVkLogicalDevice(){
    std::vector<std::string> deviceExtensions = gatherDeviceExtensions();

    VkSurfaceKHR presentaionSurface = VK_NULL_HANDLE;
    if(mPresentationProvider != nullptr){
        assert(mPresentationProvider->getProviderTypeBitmask() & VulkanProviderTypeEnum::PRESENTATION_BIT);
        presentaionSurface = mPresentationProvider->getPresentationSurface();
    }
    mDeviceBundle.logicalDevice = mDeviceBundle.physicalDevice.createLogicalDevice(getRequiredQueueFlags(), vkutils::strings_to_cstrs(deviceExtensions), presentaionSurface);
}

void VulkanSetupCore::cleanup(){
    VmaHost::destroyAllocator(VulkanDeviceHandlePair(mDeviceBundle));
    vkDestroyDevice(mDeviceBundle.logicalDevice.handle(), nullptr);
    vkDestroyInstance(mVkInstance, nullptr);
}

#ifdef MANUAL_DEVICE_SELECTION
static VkPhysicalDevice manual_select_physical_device(const std::vector<VkPhysicalDevice>& aDevices){
    size_t selectionIndex = std::numeric_limits<size_t>::max();

    do{
        std::cout << "Please select a device:" << std::endl;
        for(size_t index = 0; index < aDevices.size(); ++index){
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(aDevices[index], &properties);
            std::cout << "  [" << index << "]: " << properties.deviceName << "(Vulkan " << properties.apiVersion  << ")" << std::endl;
        }
        std::cout << "selection: ";

        std::string input;
        std::getline(std::cin, input);
        std::istringstream ins{input};

        if((ins >> selectionIndex) || selectionIndex >= aDevices.size()){
            std::cout << "Invalid selection" << std::endl;
        }
    }while(selectionIndex >= aDevices.size());

    return(aDevices.at(selectionIndex));
}
#endif