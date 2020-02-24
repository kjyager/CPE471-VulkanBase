#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "data/VertexInput.h"
#include "utils/BufferedTimer.h"
#include "load_obj.h"
#include <iostream>
#include <memory> // Include shared_ptr
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

struct Transforms {
    alignas(16) glm::mat4 Model;
};

struct ViewTransforms {
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Perspective;
};

using UniformTransformData = UniformStructData<Transforms>;
using UniformTransformDataPtr = std::shared_ptr<UniformTransformData>;
using UniformViewData = UniformStructData<ViewTransforms>;
using UniformViewDataPtr = std::shared_ptr<UniformViewData>;

class Application : public VulkanGraphicsApp
{
 public:
    void init();
    void run();
    void cleanup();

 protected:
    void initGeometry();
    void initShaders();
    void initUniforms(); 

    void render();

    UniformDataLayoutSet mUniformLayoutSet;
    std::unordered_map<std::string, ObjMultiShapeGeometry> mObjects;
    std::unordered_map<std::string, UniformTransformDataPtr> mObjectTransforms;

    UniformViewDataPtr mViewData = nullptr;
};


int main(int argc, char** argv){
    Application* app = nullptr;
    QUICK_TIME("Application Init", app = new Application());
    app->init();
    app->run();
    app->cleanup();

    return(0);
}

void Application::init(){
    // Initialize uniform variables
    QUICK_TIME("initUniforms", initUniforms());
    // Initialize geometry 
    QUICK_TIME("initGeometry", initGeometry());
    // Initialize shaders
    QUICK_TIME("initShaders", initShaders());

    // Initialize graphics pipeline and render setup 
    VulkanGraphicsApp::init();
}

void Application::run(){
    FpsTimer globalRenderTimer(0);
    FpsTimer localRenderTimer(1024);

    GLFWwindow* window = VulkanGraphicsApp::getWindowPtr();

    // Run until the application is closed
    while(!glfwWindowShouldClose(window)){
        // Poll for window events, keyboard and mouse button presses, ect...
        glfwPollEvents();

        // Render the frame 
        globalRenderTimer.frameStart();
        localRenderTimer.frameStart();
        render();
        globalRenderTimer.frameFinish();
        localRenderTimer.frameFinish();

        // Print out framerate statistics if enough data has been collected 
        if(localRenderTimer.isBufferFull()){
            localRenderTimer.reportAndReset();
        }
    }

    std::cout << "Average Performance: " << globalRenderTimer.getReportString() << std::endl;
    
    // Make sure the GPU is done rendering before moving on. 
    vkDeviceWaitIdle(VulkanGraphicsApp::getPrimaryDeviceBundle().logicalDevice.handle());
}

void Application::cleanup(){
    // Let base class handle cleanup.
    VulkanGraphicsApp::cleanup();
}

void Application::render(){
    // Tell the GPU to render a frame. 
    VulkanGraphicsApp::render();
} 

void Application::initGeometry(){
    QUICK_TIME("cube.obj load time", mObjects["cube"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/cube.obj"));
    QUICK_TIME("suzanne.obj load time", mObjects["monkey"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/suzanne.obj"));
    QUICK_TIME("gear.obj load time", mObjects["gear"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/gear.obj"));

    mObjectTransforms["cube"] = std::make_shared<UniformTransformData>();
    mObjectTransforms["monkey"] = std::make_shared<UniformTransformData>();
    mObjectTransforms["gear"] = std::make_shared<UniformTransformData>();

    VulkanGraphicsApp::addMultiShapeObject(mObjects["cube"], {{0, mObjectTransforms["cube"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["monkey"], {{0, mObjectTransforms["monkey"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["gear"], {{0, mObjectTransforms["gear"]}});

    mObjectTransforms["cube"]->getStruct().Model = glm::translate(glm::vec3(0.0, 0.0, 0.0));
    mObjectTransforms["monkey"]->getStruct().Model = glm::translate(glm::vec3(-2.0, 0.0, 0.0));
    mObjectTransforms["gear"]->getStruct().Model = glm::translate(glm::vec3(2.0, 0.0, 0.0));
}

void Application::initShaders(){
    // Get the handle representing the GPU. 
    VkDevice logicalDevice = VulkanGraphicsApp::getPrimaryDeviceBundle().logicalDevice.handle();

    // Load the compiled shader code from disk. 
    VkShaderModule vertShader = vkutils::load_shader_module(logicalDevice, STRIFY(SHADER_DIR) "/standard.vert.spv");
    VkShaderModule fragShader = vkutils::load_shader_module(logicalDevice, STRIFY(SHADER_DIR) "/normal.frag.spv");
    
    assert(vertShader != VK_NULL_HANDLE);
    assert(fragShader != VK_NULL_HANDLE);

    VulkanGraphicsApp::setVertexShader("standard.vert", vertShader);
    VulkanGraphicsApp::setFragmentShader("vertexColor.frag", fragShader);
}

void Application::initUniforms(){

    // Specify the layout for per-object uniforms
    mUniformLayoutSet = UniformDataLayoutSet{
        // {<binding point>, <structure layout>}
        {0, UniformTransformData::sGetLayout()}
    };
    VulkanGraphicsApp::initMultiShapeUniformBuffer(mUniformLayoutSet);

    // Specify the structure of single instance uniforms
    mViewData = UniformViewData::create();
    VulkanGraphicsApp::addSingleInstanceUniform(1, mViewData);

    mViewData->getStruct().View = glm::lookAt(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));

    // Set the persepctive matrix
    VkExtent2D frameDimensions = getFramebufferSize();
    double aspect = static_cast<double>(frameDimensions.width) / static_cast<double>(frameDimensions.height);
    mViewData->getStruct().Perspective = glm::perspective(45.0, aspect, .01, 100.0);
}