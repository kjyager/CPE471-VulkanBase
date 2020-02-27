#define _DISABLE_EXTENDED_ALIGNED_STORAGE // Fix MSVC warning about binary compatability

#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "data/VertexInput.h"
#include "utils/BufferedTimer.h"
#include "load_obj.h"
#include <iostream>
#include <memory> // Include shared_ptr

#define ENABLE_GLM_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

struct Transforms {
    alignas(16) glm::mat4 Model;
};

struct ViewTransforms {
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Perspective;
    alignas(16) glm::vec3 W_ViewDir;
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
    void updateView();

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
    glfwSetInputMode(getWindowPtr(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

        updateView();

        // Render the frame 
        globalRenderTimer.frameStart();
        render();
        globalRenderTimer.frameFinish();

        // Print out framerate statistics if enough data has been collected 
        if(localRenderTimer.isBufferFull()){
            localRenderTimer.reportAndReset();
        }
    }

    std::cout << "Average Performance: " << globalRenderTimer.getReportString() << std::endl;
    
    // Make sure the GPU is done rendering before moving on. 
    vkDeviceWaitIdle(VulkanGraphicsApp::getPrimaryDeviceBundle().logicalDevice.handle());
}

void Application::updateView(){
    const static float xSensitivity = 1.0f/glm::pi<float>();
    const static float ySensitivity = xSensitivity;
    const static float thetaLimit = glm::radians(89.99f);
    static glm::dvec2 lastPos = glm::dvec2(-1.0);

    glm::dvec2 pos;
    glfwGetCursorPos(getWindowPtr(), &pos.x, &pos.y);
    glm::vec2 delta = pos - lastPos;

	std::cout << "Mouse Pos: " << glm::to_string(pos) << std::endl;
	std::cout << "Mouse delta: " <<  glm::to_string(delta) << std::endl;

    if(lastPos.x < 0.0){
        delta = glm::vec2(0.0);
    }

    lastPos = pos;

    static float theta, phi;

    phi += glm::radians(delta.x * xSensitivity);
    theta = glm::clamp(theta + glm::radians(delta.y*ySensitivity), -thetaLimit, thetaLimit);

    assert(mViewData != nullptr);

    glm::vec3 eye = 7.0f*glm::normalize(glm::vec3(cos(phi)*cos(theta), sin(theta), sin(phi)*cos(theta)));
    glm::vec3 look = glm::vec3(0.0);
    mViewData->getStruct().View = glm::lookAt(eye, look, glm::vec3(0.0, 1.0, 0.0));
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
    QUICK_TIME("bunny.obj load time", mObjects["bunny"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/bunny.obj"));
    QUICK_TIME("teapot.obj load time", mObjects["teapot"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/teapot.obj"));

    mObjectTransforms["cube"] = std::make_shared<UniformTransformData>();
    mObjectTransforms["monkey"] = std::make_shared<UniformTransformData>();
    mObjectTransforms["bunny"] = std::make_shared<UniformTransformData>();
    mObjectTransforms["teapot"] = std::make_shared<UniformTransformData>();

    VulkanGraphicsApp::addMultiShapeObject(mObjects["cube"], {{0, mObjectTransforms["cube"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["monkey"], {{0, mObjectTransforms["monkey"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["bunny"], {{0, mObjectTransforms["bunny"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["teapot"], {{0, mObjectTransforms["teapot"]}});

    using namespace glm;

    float angle = 2.0f*pi<float>()/3.0f;
    mObjectTransforms["cube"]->getStruct().Model = translate(vec3(0.0, 0.0, 0.0));
    mObjectTransforms["monkey"]->getStruct().Model = translate(2.5f*vec3(cos(0), 0.0, sin(0)));
    mObjectTransforms["bunny"]->getStruct().Model = translate(2.5f*vec3(cos(angle*1), 0.0, sin(angle*1)));
    mObjectTransforms["teapot"]->getStruct().Model = translate(2.5f*vec3(cos(angle*2), 0.0, sin(angle*2)));
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
    mViewData->getStruct().W_ViewDir = normalize(glm::vec3(0.0, 0.0, -5.0));

    // Set the persepctive matrix
    VkExtent2D frameDimensions = getFramebufferSize();
    double aspect = static_cast<double>(frameDimensions.width) / static_cast<double>(frameDimensions.height);
    glm::mat4 P = glm::perspective(45.0, aspect, .01, 100.0);
    P[1][1] *= -1;
    mViewData->getStruct().Perspective = P;
}