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
    for(auto& entry : mComputeStages){
        entry.second.pipeline.destroy(getPrimaryDeviceBundle().logicalDevice);
    }

    if(mComputeCommandPool != VK_NULL_HANDLE){
        vkDestroyCommandPool(getPrimaryDeviceBundle().logicalDevice, mComputeCommandPool, nullptr);
        mComputeCommandPool = VK_NULL_HANDLE;
    }
}

vkutils::ComputeStage& ComputeProvider::registerComputeStage(const std::string& aStageId) {
    if(hasRegisteredStage(aStageId)){
        throw ComputeStageExistsError(aStageId);
    }

    return(mComputeStages[aStageId]);
}

void ComputeProvider::registerComputeStage(const std::string& aStageId, const vkutils::ComputeStage& aComputeStage) {
    if(hasRegisteredStage(aStageId)){
        throw ComputeStageExistsError(aStageId);
    }

    mComputeStages[aStageId] = aComputeStage;
}

vkutils::ComputeStage ComputeProvider::unregisterComputeStage(const std::string& aStageId) {
    const auto& finder = mComputeStages.find(aStageId);
    if(finder == mComputeStages.end()){
        throw ComputeStageMissingError(aStageId);
    }

    vkutils::ComputeStage removedStage = finder->second;
    mComputeStages.erase(finder);
    return(removedStage);
}

bool ComputeProvider::hasRegisteredStage(const std::string& aStageId) const {
    const auto& finder = mComputeStages.find(aStageId);
    return(finder != mComputeStages.end());
}

const vkutils::ComputeStage& ComputeProvider::getComputeStage(const std::string& aStageId) const {
    const auto& finder = mComputeStages.find(aStageId);
    if(finder == mComputeStages.end()){
        throw ComputeStageMissingError(aStageId);
    }

    return(finder->second);
}
