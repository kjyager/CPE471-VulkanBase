#ifndef VULKAN_COMPUTE_PROVIDER_H_
#define VULKAN_COMPUTE_PROVIDER_H_
#include "VulkanAppInterface.h"
#include "vkutils/vkutils.h"
#include "vkutils/VulkanDevices.h"
#include <unordered_map>
#include <vector>

class ComputeStageMissingError : public std::runtime_error {
 public:
    ComputeStageMissingError(const std::string& aStageId)
    : std::runtime_error("Error: No compute stage was found matching stageId '" + aStageId + "'!")
    {}
};

class ComputeStageExistsError : public std::runtime_error {
 public:
    ComputeStageExistsError(const std::string& aStageId)
    : std::runtime_error("Error: A compute stage already exists matching stageId '" + aStageId + "'!\nStages must be unregistered before they are replaced.")
    {}
};

class ComputeProvider : virtual public ComputeProviderInterface
{
 public:
    virtual void init() override;
    virtual void cleanup() override;


    virtual VkCommandPool getCommandPool() const override {return(mComputeCommandPool);}

    virtual vkutils::ComputeStage& registerComputeStage(const std::string& aStageId) override;
    virtual void registerComputeStage(const std::string& aStageId, const vkutils::ComputeStage& aComputeStage) override;
    virtual vkutils::ComputeStage unregisterComputeStage(const std::string& aStageId) override;
    virtual bool hasRegisteredStage(const std::string& aStageId) const override;
    virtual const vkutils::ComputeStage& getComputeStage(const std::string& aStageId) const override;

 protected:

    VkCommandPool mComputeCommandPool = VK_NULL_HANDLE;
    
    std::unordered_map<std::string, vkutils::ComputeStage> mComputeStages;
};

#endif