#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/VertexInput.h"
#include "utils/FpsTimer.h"
#include <iostream>
#include <memory> // Include shared_ptr
#include <thread>

struct SimpleVertex{
    glm::vec3 pos;
    glm::vec4 color;
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

    std::shared_ptr<SimpleVertexBuffer> mGeometry = nullptr;
};

int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    return(0);
}

void Application::init(){
    // Initialize GPU devices and display setup
    VulkanSetupBaseApp::init(); 

    // Initialize geometry 
    initGeometry();

    // Initialize graphics pipeline and render setup 
    VulkanGraphicsApp::init();
}

void Application::run(){
    FpsTimer globalRenderTimer(0);
    FpsTimer localRenderTimer(1024);

    while(!glfwWindowShouldClose(mWindow)){
        glfwPollEvents();

        globalRenderTimer.frameStart();
        localRenderTimer.frameStart();
        render();
        globalRenderTimer.frameFinish();
        localRenderTimer.frameFinish();

        if(localRenderTimer.isBufferFull()){
            localRenderTimer.reportAndReset();
        }
        ++mFrameNumber;
    }

    std::string globalReport = globalRenderTimer.getReportString();
    std::cout << "Average Performance: " << globalReport << std::endl;
    
    vkDeviceWaitIdle(mLogicalDevice.handle());
}

void Application::cleanup(){
    mGeometry->freeBuffer();
    mGeometry.reset();
    VulkanGraphicsApp::cleanup();
}

void Application::render(){
    VulkanGraphicsApp::render();
}

void Application::initGeometry(){
    const static std::vector<SimpleVertex> triangleVerts = {
        {glm::vec3(-0.5, 0.5, 0.0), glm::vec4(1.0, 0.0, 0.0, 1.0)},
        {glm::vec3(0.0, -.5, 0.0), glm::vec4(0.0, 1.0, 0.0, 1.0)},
        {glm::vec3(0.5, 0.5, 0.0), glm::vec4(0.0, 0.0, 1.0, 1.0)},
    };
    VulkanDeviceHandlePair deviceInfo = {mLogicalDevice.handle(), mPhysDevice.handle()};

    mGeometry = std::make_shared<SimpleVertexBuffer>(triangleVerts, deviceInfo);

    assert(mGeometry->getDeviceSyncState() == SimpleVertexBuffer::DEVICE_IN_SYNC);

    const static SimpleVertexInput vtxInput( /*binding = */ 0U,
        /*vertex attribute descriptions = */ {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SimpleVertex, pos)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SimpleVertex, color)}
        }
    );

    VulkanGraphicsApp::setVertexInput(vtxInput.getBindingDescription(), vtxInput.getAttributeDescriptions());
    VulkanGraphicsApp::setVertexBuffer(mGeometry->handle(), mGeometry->vertexCount());

}
void Application::initShaders(){

}