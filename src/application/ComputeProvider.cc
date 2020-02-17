#include "ComputeProvider.h"

void ComputeProvider::init(){
    VkCommandPoolCreateInfo commandPoolInfo = {};
    {
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = nullptr;
        commandPoolInfo.flags = 0;
        commandPoolInfo.queueFamilyIndex = getPrimaryDeviceBundle().physicalDevice.mComputeIdx.value();
    }
    
    if(vkCreateCommandPool(getPrimaryDeviceBundle().logicalDevice, &commandPoolInfo, nullptr, &mComputeCommandPool) != VK_SUCCESS){
        throw std::runtime_error("Failed to create command pool for compute app!");
    }
}

void ComputeProvider::cleanup(){
    for(auto& entry : mComputePipelines){
        entry.second.destroy(getPrimaryDeviceBundle().logicalDevice);
    }

    for(auto& entry : mShaderModules){
        vkDestroyShaderModule(getPrimaryDeviceBundle().logicalDevice, entry.second, nullptr);
    }

    if(mComputeCommandPool != VK_NULL_HANDLE){
        vkDestroyCommandPool(getPrimaryDeviceBundle().logicalDevice, mComputeCommandPool, nullptr);
        mComputeCommandPool = nullptr;
    }
}