#ifndef VULKAN_GRAPHICS_APP_H_
#define VULKAN_GRAPHICS_APP_H_
#include "application/VulkanSetupCore.h"
#include "application/SwapchainProvider.h"
#include "application/RenderProvider.h"
#include "vkutils/vkutils.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "load_obj.h"
#include "utils/common.h"
#include <map>
#include <memory>

class VulkanGraphicsApp : virtual public VulkanAppInterface, public CoreLink{
 public:
    /// Default constructor runs full initCore() immediately. Use protected no-init constructor
    /// it this is undesirable. 
    VulkanGraphicsApp() {initCore();}

    void init();
    void render();
    void cleanup();
    
    GLFWwindow* getWindowPtr() const {return(mSwapchainProvider->getWindowPtr());}
    const VkExtent2D& getFramebufferSize() const;
    size_t getFrameNumber() const {return(mFrameNumber);}

    /// Add MultiShapeGeometry loaded from an OBJ file with uniform data 'aUniformData'
    void addMultiShapeToScene(const ObjMultiShapeGeometry& aMultiShape, uint32_t aBindPoint, UniformDataInterfacePtr aUniformdata);
    /// Add MultiShapeGeometry loaded from an OBJ file with uniform data from 'aUniformData'
    void addMultiShapeToScene(const ObjMultiShapeGeometry& aMultiShape, const UniformDataInterfaceSet& aUniformdata);

    void setVertexShader(const std::string& aShaderName, const VkShaderModule& aShaderModule);
    void setFragmentShader(const std::string& aShaderName, const VkShaderModule& aShaderModule);

 protected:
    friend class VulkanProviderInterface;
    
    virtual const VkApplicationInfo& getAppInfo() const override;
    virtual const std::vector<std::string>& getRequestedValidationLayers() const override;

#ifdef CPE471_VULKAN_SAFETY_RAILS
 private:
#else
 protected:
#endif

    VulkanGraphicsApp(bool noInitCore){if(!noInitCore) initCore();}

    void initCore(); 
    
    void initRenderPipeline();
    void initFramebuffers();
    void initCommands();
    void initSync();

    void resetRenderSetup();
    void cleanupSwapchainDependents();

    void initUniformBuffer();
    void initUniformDescriptorPool();
    void initUniformDescriptorSets();

    void reinitUniformResources();

    size_t mFrameNumber = 0;

    std::shared_ptr<VulkanSetupCore> mCoreProvider = nullptr; // Shadows CoreLink::mCoreProvider 
    std::shared_ptr<SwapchainProvider> mSwapchainProvider = nullptr;

    const static int IN_FLIGHT_FRAME_LIMIT = 2;
    std::vector<VkFramebuffer> mSwapchainFramebuffers;
    std::vector<VkSemaphore> mImageAvailableSemaphores;
    std::vector<VkSemaphore> mRenderFinishSemaphores;
    std::vector<VkFence> mInFlightFences;

    vkutils::VulkanBasicRasterPipelineBuilder mRenderPipeline;

    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    std::unordered_map<std::string, VkShaderModule> mShaderModules;
    std::string mVertexKey;
    std::string mFragmentKey;

    struct MultiShapeBundle{ObjMultiShapeGeometry multiShape; UniformDataInterfaceSet uniformSet;};
    std::vector<ObjMultiShapeGeometry> mMultiShapeObjects;

    UniformBuffer mUniformBuffer;
    VkDeviceSize mTotalUniformDescriptorSetCount = 0;
    VkDescriptorPool mResourceDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> mUniformDescriptorSetLayouts;
    std::vector<VkDescriptorSet> mUniformDescriptorSets;
};

#endif
