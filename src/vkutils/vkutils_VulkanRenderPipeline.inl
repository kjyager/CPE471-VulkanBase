struct VulkanSwapchainBundle
{
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR presentation_mode;
    VkExtent2D extent = {0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t requested_image_count = 0;
    uint32_t image_count = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

struct VulkanDepthBundle{
    VkImage depthImage = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo mAllocInfo = {};
    VkFormat format;
};

class VulkanRenderPipeline
{
 public:
    /** Default constructor always creates invalid and empty 
     * instance. Use a builder class to create valid instace
     */
    VulkanRenderPipeline() = default;

    bool isValid() const {
        return(
            mGraphicsPipeline != VK_NULL_HANDLE &&
            mGraphicsPipeLayout != VK_NULL_HANDLE &&
            mRenderPass != VK_NULL_HANDLE
        );
    }

    // Destroy this pipeline and associated Vulkan objects
    void destroy();

    const VkPipeline& handle() const { return(mGraphicsPipeline); }
    const VkPipelineLayout& getLayout() const { return(mGraphicsPipeLayout); }
    const VkRenderPass& getRenderpass() const { return(mRenderPass); }
    const VkViewport& getViewport() const { return(mViewport); }

 protected:

    VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mGraphicsPipeLayout = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkViewport mViewport;

    VkDevice _mLogicalDevice = VK_NULL_HANDLE;
};

class RenderPassConstructionSet
{
 public:

    // Device handle needed for VkCreate functions
    VulkanDeviceHandlePair mDevicePair; 

    // Swapchain information is needed for setting up parts of 
    // the render pass. 
    VulkanSwapchainBundle const* mSwapchainBundle = nullptr;

    VkAttachmentDescription mColorAttachment = {};
    VkAttachmentDescription mDepthAttachment = {};

    VkAttachmentReference mColorAttachmentRef = {};
    VkAttachmentReference mDepthAttachmentRef = {};
    VkSubpassDescription mSubpass = {};
    VkSubpassDependency mDependency = {};

 protected:
    friend class GraphicsPipelineConstructionSet;
    friend class VulkanBasicRasterPipelineBuilder;
    RenderPassConstructionSet(){}
    RenderPassConstructionSet(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle)
    :   mDevicePair(aDevicePair), mSwapchainBundle(aChainBundle) {}
};

class GraphicsPipelineConstructionSet
{
 public:
    // Device handle needed for VkCreate functions
    VulkanDeviceHandlePair mDevicePair;

    // Swapchain information is needed for setting up parts of 
    // the pipeline. 
    VulkanSwapchainBundle const* mSwapchainBundle = nullptr;

    // Optional depth buffer bundle for enabling depth testing
    VulkanDepthBundle mDepthBundle;

    // Sub construction set for the render pass
    RenderPassConstructionSet mRenderpassCtorSet;

    // It's assumed all programmable stages will be added by the user.
    std::vector<VkPipelineShaderStageCreateInfo> mProgrammableStages;

    // It's assumed most of these will be set up using boiler plate code
    // Intervention from the user is not expected, but is certainly allowed
    VkPipelineVertexInputStateCreateInfo mVtxInputInfo;
    VkPipelineInputAssemblyStateCreateInfo mInputAsmInfo;
    VkViewport mViewport;
    VkRect2D mScissor;
    VkPipelineRasterizationStateCreateInfo mRasterInfo;
    VkPipelineMultisampleStateCreateInfo mMultisampleInfo;
    VkPipelineColorBlendAttachmentState mBlendAttachmentInfo;
    VkPipelineColorBlendStateCreateInfo mColorBlendInfo;
    VkPipelineLayoutCreateInfo mPipelineLayoutInfo;
    VkPipelineDepthStencilStateCreateInfo mDepthStencilInfo;
    std::vector<VkDynamicState> mDynamicStates;

 protected:
    friend class VulkanBasicRasterPipelineBuilder;
    GraphicsPipelineConstructionSet(){}
    GraphicsPipelineConstructionSet(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle)
    : mDevicePair(aDevicePair), mSwapchainBundle(aChainBundle), mRenderpassCtorSet(aDevicePair, aChainBundle) {}

};

class VulkanBasicRasterPipelineBuilder : public VulkanRenderPipeline
{
 public:
    VulkanBasicRasterPipelineBuilder() = default;
   
    /// Setup construction set during object construction.
    VulkanBasicRasterPipelineBuilder(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle);

    /// Setup and return a default construction set as a non-const reference
    GraphicsPipelineConstructionSet& setupConstructionSet(const VulkanDeviceHandlePair& aDevicePair, const VulkanSwapchainBundle* aChainBundle);

    /// Fill in the construction set with boilerplate values for a graphics render pipeline
    /// Modifies the given construction set, but does not actual object creation such that 
    /// tweaks can be made to the construction set prior to actual pipeline creation. 
    static void prepareFixedStages(GraphicsPipelineConstructionSet& aCtorSetInOut);
    static void prepareViewport(GraphicsPipelineConstructionSet& aCtorSetInOut);
    static void prepareRenderPass(GraphicsPipelineConstructionSet& aCtorSetInOut);

    /// Automatically select an appropriate depth buffer configuration based on aCtorSet and return the created depth buffer
    /// NOTE: An swapchain bundle must be bound to the construction set. 
    static VulkanDepthBundle autoCreateDepthBuffer(const GraphicsPipelineConstructionSet& aCtorSet);
    
    /// Automatically select an appropriate depth buffer configuration based on the internal construction set and return the created depth buffer
    /// NOTE: An swapchain bundle must be bound to the construction set. 
    VulkanDepthBundle autoCreateDepthBuffer() const;

    /// Submit aFinalCtorSet as the construction set for this pipeline. The pipeline
    /// is then created fresh using the given construction set. The success of this
    /// function will make the object valid and usable. 
    void build(const GraphicsPipelineConstructionSet& aFinalCtorSet);

    /// Create pipeline using internal construction set. The success of this
    /// function will make the object valid and usable. 
    void build() {build(_mConstructionSet);}

    /// Recreate the pipeline using the existing construction set. Swapchain information should
    /// still be accessible through the pointer given during construction of this object, so 
    /// an out of sync swapchain should be re-synchronized automatically during recreation. 
    void rebuild();

 private:
    GraphicsPipelineConstructionSet _mConstructionSet;
};