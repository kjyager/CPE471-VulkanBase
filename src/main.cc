#define _DISABLE_EXTENDED_ALIGNED_STORAGE // Fix MSVC warning about binary compatability

#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "data/VertexInput.h"
#include "utils/BufferedTimer.h"
#include "load_obj.h"
#include <iostream>
#include <limits>
#include <memory> // Include shared_ptr

#define ENABLE_GLM_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

struct Transforms {
    alignas(16) glm::mat4 Model;
};

struct WorldInfo {
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Perspective;
    alignas(16) float time = 0.0f;
};

struct AnimShadeData {
    alignas(16) int shadeStyle = 0;
};

using UniformTransformData = UniformStructData<Transforms>;
using UniformTransformDataPtr = std::shared_ptr<UniformTransformData>;
using UniformWorldInfo = UniformStructData<WorldInfo>;
using UniformWorldInfoPtr = std::shared_ptr<UniformWorldInfo>;
using UniformAnimShadeData = UniformStructData<AnimShadeData>;
using UniformAnimShadeDataPtr = std::shared_ptr<UniformAnimShadeData>;

class Application : public VulkanGraphicsApp
{
 public:
    void init();

    void run();
    void updateView();

    void cleanup();

    static void resizeCallback(GLFWwindow* aWindow, int aWidth, int aHeight);
    static void scrollCallback(GLFWwindow* aWindow, double aXOffset, double aYOffset);
    static void keyCallback(GLFWwindow* aWindow, int key, int scancode, int action, int mods);

 protected:
    void initGeometry();
    void initShaders();
    void initUniforms();

    void render(double dt);

    UniformDataLayoutSet mUniformLayoutSet;
    std::unordered_map<std::string, ObjMultiShapeGeometry> mObjects;
    std::unordered_map<std::string, UniformTransformDataPtr> mObjectTransforms;
    std::unordered_map<std::string, UniformAnimShadeDataPtr> mObjectAnimShade;

    UniformWorldInfoPtr mWorldInfo = nullptr;

    static float smViewZoom;
    static bool smResizeFlag;
};

float Application::smViewZoom = 7.0f;
bool Application::smResizeFlag = false;

void Application::resizeCallback(GLFWwindow* aWindow, int aWidth, int aHeight){
    smResizeFlag = true;
}

void Application::scrollCallback(GLFWwindow* aWindow, double aXOffset, double aYOffset){
    const float scrollSensitivity = 1.0f;
    smViewZoom = glm::clamp(smViewZoom + float(-aYOffset)*scrollSensitivity, 2.0f, 30.0f);
}

void Application::keyCallback(GLFWwindow* aWindow, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        int mode = glfwGetInputMode(aWindow, GLFW_CURSOR);
        if(mode != GLFW_CURSOR_DISABLED){
            glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }else{
            glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
    }
    else if((key == GLFW_KEY_F11 || key == GLFW_KEY_F) && action == GLFW_PRESS)
    {
        GLFWmonitor* monitor = glfwGetWindowMonitor(aWindow);
        static int winLastWidth = 854, winLastHeight = 480;
        
        if(monitor == nullptr){
            // Not fullscreen. Go fullscreen...
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if(mode == nullptr){
                std::cerr << "Warning! Unable to go fullscreen because of missing monitor information!" << std::endl;
                return;
            }
            glfwGetWindowSize(aWindow, &winLastWidth, &winLastHeight);
            glfwSetWindowMonitor(aWindow, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
        }else{
            // Fullscreen, go back to windowed.
            glfwSetWindowMonitor(aWindow, nullptr, 0, 0, winLastWidth, winLastHeight, GLFW_DONT_CARE);
        }
    }
}


int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    return(0);
}

void Application::init(){

    // Set cursor callback and shortcut to grab mouse cursor
    glfwSetWindowSizeCallback(getWindowPtr(), resizeCallback);
    glfwSetScrollCallback(getWindowPtr(), scrollCallback);
    glfwSetKeyCallback(getWindowPtr(), keyCallback);

    // Initialize uniform variables
    initUniforms();
    // Initialize geometry 
    initGeometry();
    // Initialize shaders
    initShaders();

    // Initialize graphics pipeline and render setup 
    VulkanGraphicsApp::init();
}

void Application::run(){
    FpsTimer globalRenderTimer(0);

    GLFWwindow* window = VulkanGraphicsApp::getWindowPtr();

    // Run until the application is closed
    while(!glfwWindowShouldClose(window)){
        // Poll for window events, keyboard and mouse button presses, ect...
        glfwPollEvents();

        // Update view information
        updateView();

        // Render the frame 
        globalRenderTimer.frameStart();
        render(globalRenderTimer.lastStepTime()*1e-6);
        globalRenderTimer.frameFinish();

        if(smResizeFlag){
            VkExtent2D frameDimensions = getFramebufferSize();
            double aspect = static_cast<double>(frameDimensions.width) / static_cast<double>(frameDimensions.height);
            glm::mat4 P = glm::perspective(glm::radians(75.0), aspect, .01, 100.0);
            P[1][1] *= -1; // Vulkan uses a flipped y-axis
            mWorldInfo->getStruct().Perspective = P;
            smResizeFlag = false;
        }
    }

    std::cout << "Average Performance: " << globalRenderTimer.getReportString() << std::endl;
    
    // Make sure the GPU is done rendering before exiting. 
    vkDeviceWaitIdle(VulkanGraphicsApp::getPrimaryDeviceBundle().logicalDevice.handle());
}

void Application::updateView(){
    const static float xSensitivity = 1.0f/glm::pi<float>();
    const static float ySensitivity = xSensitivity;
    const static float thetaLimit = glm::radians(89.99f);
    static glm::dvec2 lastPos = glm::dvec2(std::numeric_limits<double>::quiet_NaN());

    glm::dvec2 pos;
    glfwGetCursorPos(getWindowPtr(), &pos.x, &pos.y);
    glm::vec2 delta = pos - lastPos;

    if(glm::isnan(lastPos.x)){
        delta = glm::vec2(0.0);
    }

    lastPos = pos;

    static float theta, phi;

    phi += glm::radians(delta.x * xSensitivity);
    theta = glm::clamp(theta + glm::radians(delta.y*ySensitivity), -thetaLimit, thetaLimit);

    assert(mWorldInfo != nullptr);

    glm::vec3 eye = smViewZoom*glm::normalize(glm::vec3(cos(phi)*cos(theta), sin(theta), sin(phi)*cos(theta)));
    glm::vec3 look = glm::vec3(0.0);
    mWorldInfo->getStruct().View = glm::lookAt(eye, look, glm::vec3(0.0, 1.0, 0.0));
}

void Application::cleanup(){
    // Let base class handle cleanup.
    VulkanGraphicsApp::cleanup();
}

void Application::render(double dt){
    using glm::sin;
    using glm::cos;
    using glm::vec3;

    static UniformTransformDataPtr logoTfs = mObjectTransforms["vulkan"];
    static UniformTransformDataPtr monkeyTfs = mObjectTransforms["monkey"];
    static UniformTransformDataPtr bunnyTfs = mObjectTransforms["bunny"];
    static UniformTransformDataPtr teapotTfs = mObjectTransforms["teapot"];

    // Global time
    float gt = static_cast<float>(glfwGetTime());
    mWorldInfo->getStruct().time = gt;

    logoTfs->getStruct().Model = glm::scale(vec3(2.5f)) * glm::rotate(float(gt), vec3(0.0, 1.0, 0.0));

    float angle = 2.0f*glm::pi<float>()/3.0f; // 120 degrees
    float radius = 4.5f;
    monkeyTfs->getStruct().Model = glm::rotate(-float(gt), vec3(0.0, 1.0, 0.0)) * glm::translate(radius*vec3(cos(angle*0), .2f*sin(gt*4.0f+angle*0), sin(angle*0))) * glm::rotate(2.0f*float(gt), vec3(0.0, 1.0, 0.0));
    bunnyTfs->getStruct().Model  = glm::rotate(-float(gt), vec3(0.0, 1.0, 0.0)) * glm::translate(radius*vec3(cos(angle*1), .2f*sin(gt*4.0f+angle*1), sin(angle*1))) * glm::rotate(2.0f*float(gt), vec3(0.0, 1.0, 0.0));
    teapotTfs->getStruct().Model = glm::rotate(-float(gt), vec3(0.0, 1.0, 0.0)) * glm::translate(radius*vec3(cos(angle*2), .2f*sin(gt*4.0f+angle*2), sin(angle*2))) * glm::rotate(2.0f*float(gt), vec3(0.0, 1.0, 0.0));

    // Tell the GPU to render a frame. 
    VulkanGraphicsApp::render();
} 

void Application::initGeometry(){
    mObjects["vulkan"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/vulkan.obj");
    mObjects["monkey"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/suzanne.obj");
    mObjects["bunny"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/bunny.obj");
    mObjects["teapot"] = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/teapot.obj");

    mObjectTransforms["vulkan"] = UniformTransformData::create();
    mObjectTransforms["monkey"] = UniformTransformData::create();
    mObjectTransforms["bunny"] = UniformTransformData::create();
    mObjectTransforms["teapot"] = UniformTransformData::create();

    mObjectAnimShade["vulkan"] = UniformAnimShadeData::create();
    mObjectAnimShade["monkey"] = UniformAnimShadeData::create();
    mObjectAnimShade["bunny"] = UniformAnimShadeData::create();
    mObjectAnimShade["teapot"] = UniformAnimShadeData::create();

    mObjectAnimShade["vulkan"]->getStruct().shadeStyle = 1;

    VulkanGraphicsApp::addMultiShapeObject(mObjects["vulkan"], {{1, mObjectTransforms["vulkan"]}, {2, mObjectAnimShade["vulkan"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["monkey"], {{1, mObjectTransforms["monkey"]}, {2, mObjectAnimShade["monkey"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["bunny"], {{1, mObjectTransforms["bunny"]}, {2, mObjectAnimShade["bunny"]}});
    VulkanGraphicsApp::addMultiShapeObject(mObjects["teapot"], {{1, mObjectTransforms["teapot"]}, {2, mObjectAnimShade["teapot"]}});
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

    // Specify the structure of single instance uniforms
    mWorldInfo = UniformWorldInfo::create();
    VulkanGraphicsApp::addSingleInstanceUniform(0, mWorldInfo);

    // Specify the layout for per-object uniforms
    mUniformLayoutSet = UniformDataLayoutSet{
        // {<binding point>, <structure layout>}
        {1, UniformTransformData::sGetLayout()},
        {2, UniformAnimShadeData::sGetLayout()}
    };
    VulkanGraphicsApp::initMultiShapeUniformBuffer(mUniformLayoutSet);

    updateView();

    // Set the perspective matrix
    VkExtent2D frameDimensions = getFramebufferSize();
    double aspect = static_cast<double>(frameDimensions.width) / static_cast<double>(frameDimensions.height);
    glm::mat4 P = glm::perspective(glm::radians(75.0), aspect, .01, 100.0);
    P[1][1] *= -1; // Vulkan uses a flipped y-axis
    mWorldInfo->getStruct().Perspective = P;
}
