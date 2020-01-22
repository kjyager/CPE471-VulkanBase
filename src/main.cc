#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "data/VertexInput.h"
#include "utils/FpsTimer.h"
#include <iostream>
#include <memory> // Include shared_ptr
#include <thread>

struct SimpleVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct Transforms {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Perspective;
};

using SimpleVertexBuffer = VertexAttributeBuffer<SimpleVertex>;
using SimpleVertexInput = VertexInputTemplate<SimpleVertex>;

class Application : public VulkanGraphicsApp
{
 public:
    void init();
    void run();
    void cleanup();

 protected:
    void initGeometry();
    void initShaders();

    void render();

    glm::vec2 getMousePos();

    std::shared_ptr<SimpleVertexBuffer> mGeometry = nullptr;
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
    if(glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        return previous;

    double posX, posY;
    glfwGetCursorPos(mWindow, &posX, &posY);

    // Get width and height of window as 2D vector 
    VkExtent2D frameExtent = getFramebufferSize();

    //lab 4: FIX this
    glm::vec2 cursorPosDeviceCoords = glm::vec2(0.0);
    glm::vec2 cursorVkCoords = previous = cursorPosDeviceCoords;
    return(cursorVkCoords);
}



void Application::init(){
    // Initialize GPU devices and display setup
    VulkanSetupBaseApp::init(); 

    // Initialize geometry 
    initGeometry();
    // Initialize shaders
    initShaders();

    // Initialize graphics pipeline and render setup 
    VulkanGraphicsApp::init();
}

void Application::run(){
    FpsTimer globalRenderTimer(0);
    FpsTimer localRenderTimer(1024);
    
    // Run until the application is closed
    while(!glfwWindowShouldClose(mWindow)){
        // Poll for window events, keyboard and mouse button presses, ect...
        glfwPollEvents();

        // Set the position of the top vertex 
        if(glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            glm::vec2 mousePos = getMousePos();
            mGeometry->getVertices()[1].pos = glm::vec3(mousePos, 0.0);
            mGeometry->updateDevice();
            VulkanGraphicsApp::setVertexBuffer(mGeometry->getBuffer(), mGeometry->vertexCount());
        }
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
        ++mFrameNumber;
    }

    std::cout << "Average Performance: " << globalRenderTimer.getReportString() << std::endl;
    
    // Make sure the GPU is done rendering before moving on. 
    vkDeviceWaitIdle(mLogicalDevice.handle());
}

void Application::cleanup(){
    // Deallocate the buffer holding our geometry and delete the buffer
    mGeometry->freeBuffer();
    mGeometry = nullptr;

    VulkanGraphicsApp::cleanup();
}

void Application::render(){
    VulkanGraphicsApp::render();
}

void Application::initGeometry(){
    //lab 4: add two other triangles (see lab write up)
    // Define the vertex positions and colors for our triangle
    const static std::vector<SimpleVertex> triangleVerts = {
        {
            glm::vec3(-0.5, -0.5, 0.0), // Bottom-left: v0
            glm::vec4(1.0, 1.0, 0.0, 1.0) // yellow
        },
        {
            glm::vec3(0.0, .5, 0.0), // Top-middle: v1
            glm::vec4(1.0, 0.0, 0.9, 1.0) // pink
        },
        {
            glm::vec3(0.5, -0.5, 0.0), // Bottom-right: v2
            glm::vec4(0.7, 0.0, 0.9, 1.0) // purply
        }
    };

    // Get references to the GPU we are using. 
    // TODO: Abstract slightly more to hide logical vs physical devices
    VulkanDeviceHandlePair deviceInfo = {mLogicalDevice.handle(), mPhysDevice.handle()};

    // Create a new vertex buffer on the GPU using the given geometry 
    mGeometry = std::make_shared<SimpleVertexBuffer>(triangleVerts, deviceInfo);

    // Check to make sure the geometry was uploaded to the GPU correctly. 
    assert(mGeometry->getDeviceSyncState() == DEVICE_IN_SYNC);
    // Specify that we wish to render this vertex buffer
    VulkanGraphicsApp::setVertexBuffer(mGeometry->handle(), mGeometry->vertexCount());

    // Define a description of the layout of the geometry data
    const static SimpleVertexInput vtxInput( /*binding = */ 0U,
        /*vertex attribute descriptions = */ {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SimpleVertex, pos)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SimpleVertex, color)}
        }
    );
    // Send this description to the GPU so that it knows how to interpret our vertex buffer 
    VulkanGraphicsApp::setVertexInput(vtxInput.getBindingDescription(), vtxInput.getAttributeDescriptions());

}

void Application::initShaders(){

    // Load the already compiled shader code from disk. 
    VkShaderModule vertShader = vkutils::load_shader_module(mLogicalDevice.handle(), STRIFY(SHADER_DIR) "/passthru.vert.spv");
    VkShaderModule fragShader = vkutils::load_shader_module(mLogicalDevice.handle(), STRIFY(SHADER_DIR) "/vertexColor.frag.spv");
    
    assert(vertShader != VK_NULL_HANDLE);
    assert(fragShader != VK_NULL_HANDLE);

    VulkanGraphicsApp::setVertexShader("passthru.vert", vertShader);
    VulkanGraphicsApp::setFragmentShader("vertexColor.frag", fragShader);
}