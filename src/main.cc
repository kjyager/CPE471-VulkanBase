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

struct SimpleVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

using SimpleVertexBuffer = HostVisVertexAttrBuffer<SimpleVertex>;
using SimpleVertexInput = VertexInputTemplate<SimpleVertex>;

struct Transforms {
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 Perspective;
};

struct AnimationInfo {
    alignas(16) float time;
};

using UniformTransformData = UniformStructData<Transforms>;
using UniformTransformDataPtr = std::shared_ptr<UniformTransformData>;
using UniformAnimationData = UniformStructData<AnimationInfo>;
using UniformAnimationDataPtr = std::shared_ptr<UniformAnimationData>;

static glm::mat4 getOrthographicProjection(const VkExtent2D& frameDim);

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

    glm::vec2 getMousePos();

    std::shared_ptr<SimpleVertexBuffer> mGeometry = nullptr;
    UniformTransformDataPtr mTransformUniforms = nullptr;
    UniformAnimationDataPtr mAnimationUniforms = nullptr;
};


int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    return(0);
}


glm::vec2 Application::getMousePos(){
    static glm::vec2 previous = glm::vec2(0.0);

    GLFWwindow* window = VulkanGraphicsApp::getWindowPtr();
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        return previous;

    double posX, posY;
    glfwGetCursorPos(window, &posX, &posY);

    // Get width and height of window as 2D vector 
    VkExtent2D frameExtent = getFramebufferSize();

    //lab 4: FIX this
    glm::vec2 cursorPosDeviceCoords = glm::vec2(0.0);
    glm::vec2 cursorVkCoords = previous = cursorPosDeviceCoords;
    return(cursorVkCoords);
}

void Application::init(){
    // Initialize geometry 
    initGeometry();
    // Initialize shaders
    initShaders();
    // Initialize shader uniform variables
    initUniforms();

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
    // Deallocate the buffer holding our geometry and delete the buffer
    mGeometry->freeAndReset();
    mGeometry = nullptr;

    mTransformUniforms = nullptr;
    mAnimationUniforms = nullptr;

    VulkanGraphicsApp::cleanup();
}

void Application::render(){

    GLFWwindow* window = VulkanGraphicsApp::getWindowPtr();

    float time = static_cast<float>(glfwGetTime());
    VkExtent2D frameDimensions = getFramebufferSize();

    // Set the value of our uniform variable
    mTransformUniforms->pushUniformData({
        glm::translate(glm::vec3(.1*cos(time), .1*sin(time), 0.0)),
        getOrthographicProjection(frameDimensions)
    });
    mAnimationUniforms->pushUniformData({time});

    // Tell the GPU to render a frame. 
    VulkanGraphicsApp::render();
} 

void Application::initGeometry(){

    ObjMultiShapeGeometry cube;
    ObjMultiShapeGeometry monkey;
    ObjMultiShapeGeometry gear;

    QUICK_TIME("cube.obj load time", cube = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/cube.obj"));
    QUICK_TIME("suzanne.obj load time", monkey = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/suzanne.obj"));
    QUICK_TIME("gear.obj load time", gear = load_obj_to_vulkan(getPrimaryDeviceBundle(), STRIFY(ASSET_DIR) "/gear.obj"));

}

void Application::initShaders(){
    // Get the handle representing the GPU. 
    VkDevice logicalDevice = VulkanGraphicsApp::getPrimaryDeviceBundle().logicalDevice.handle();

    // Load the compiled shader code from disk. 
    VkShaderModule vertShader = vkutils::load_shader_module(logicalDevice, STRIFY(SHADER_DIR) "/standard.vert.spv");
    VkShaderModule fragShader = vkutils::load_shader_module(logicalDevice, STRIFY(SHADER_DIR) "/vertexColor.frag.spv");
    
    assert(vertShader != VK_NULL_HANDLE);
    assert(fragShader != VK_NULL_HANDLE);

    VulkanGraphicsApp::setVertexShader("standard.vert", vertShader);
    VulkanGraphicsApp::setFragmentShader("vertexColor.frag", fragShader);
}

void Application::initUniforms(){
    mTransformUniforms = UniformTransformData::create();
    mAnimationUniforms = UniformAnimationData::create(); 
}

static glm::mat4 getOrthographicProjection(const VkExtent2D& frameDim){
    float left, right, top, bottom;
    left = top = -1.0;
    right = bottom = 1.0;

    if(frameDim.width > frameDim.height){
        float aspect = static_cast<float>(frameDim.width) / frameDim.height;
        left *= aspect;
        right *= aspect;
    }else{
        float aspect = static_cast<float>(frameDim.height) / frameDim.width;
        top *= aspect;
        right *= aspect;
    }

    return(glm::ortho(left, right, bottom, top));
}