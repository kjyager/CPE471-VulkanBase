#include "VulkanGraphicsApp.h"
#include "data/VertexGeometry.h"
#include "data/UniformBuffer.h"
#include "data/VertexInput.h"
#include "utils/BufferedTimer.h"
#include <iostream>
#include <memory> // Include shared_ptr
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
// The code below defines the types and formatting for different data we will be using in to define,
// draw, and shade our geometry. Somewhat analogous to vertex attributes and uniform variables in
// OpenGL, but with more defined structures. 
///////////////////////////////////////////////////////////////////////////////////////////////////

struct SimpleVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

// Creates a vertex attribute buffer TYPE specific to the SimpleVertex struct. 
// This will be instantiated later.
using SimpleVertexBuffer = VertexAttributeBuffer<SimpleVertex>;

// Creates a special type specific to the SimpleVertex struct which will later
// be used to tell the graphics card how to interpret our vertex data. 
using SimpleVertexInput = VertexInputTemplate<SimpleVertex>;

// Struct laying out the contents of uniform variables available to our shaders.
// For now it just contains the orthorgraphic projection matrix.
struct WorldInfo {
    alignas(16) glm::mat4 Ortho;
};

// Creates a wrapper type around the WorldInfo struct which allows the struct
// to be used as uniform data in our shaders. 
using UniformTransformData = UniformStructData<WorldInfo>;
// Defines a pointer type for the previous definition for convenience. 
using UniformTransformDataPtr = std::shared_ptr<UniformTransformData>;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

static glm::mat4 getOrthographicProjection(const VkExtent2D& frameDim);

/// Our application class. Pay attention to the member variables defined at the end. 
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

    // Explicitly inheriting the Vulkan device handles from parent class for clarity
    using VulkanGraphicsApp::mDeviceBundle;

    /// The vertex buffer that will contain our application's triangle geometry
    std::shared_ptr<SimpleVertexBuffer> mGeometry = nullptr;

    /// An wrapped instance of struct WorldInfo made available as uniform data in our shaders.
    UniformTransformDataPtr mWorldUniforms = nullptr;
};


int main(int argc, char** argv){
    Application app;
    app.init();
    app.run();
    app.cleanup();

    return(0);
}

/// Function to get and return mouse postion in device coordinates.
/// Currently broken, you get to fix it!
glm::vec2 Application::getMousePos(){
    static glm::vec2 previous = glm::vec2(0.0);
    if(glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_1) != GLFW_PRESS)
        return previous;

    double posX, posY;
    glfwGetCursorPos(mWindow, &posX, &posY);

    // Get width and height of window as 2D vector 
    VkExtent2D frameExtent = getFramebufferSize();

    //lab: FIX this
    glm::vec2 cursorPosDeviceCoords = glm::vec2(0.0);
    glm::vec2 cursorVkCoords = previous = cursorPosDeviceCoords;
    return(cursorVkCoords);
}

/// Initialize our hello triangle application
void Application::init(){
    // Initialize GPU devices and display setup behind the scenes
    VulkanSetupBaseApp::init(); 

    // Initialize geometry 
    initGeometry();
    // Initialize shaders
    initShaders();
    // Initialize shader uniform variables
    initUniforms();

    // Initialize graphics pipeline and render setup
    VulkanGraphicsApp::init();
}

/// The main loop of our application.
void Application::run(){

    // Run until the application is closed
    while(!glfwWindowShouldClose(mWindow)){
        // Poll for window events, keyboard and mouse button presses, ect...
        glfwPollEvents();

        // Render the frame 
        render();

        ++mFrameNumber;
    }

    // Make sure the GPU is done rendering before exiting. 
    vkDeviceWaitIdle(mDeviceBundle.logicalDevice.handle());
}

// Cleanup all resources used by our hello triangle application
void Application::cleanup(){
    // Deallocate the buffer holding our geometry and delete the buffer
    mGeometry->freeBuffer();
    mGeometry = nullptr;

    mWorldUniforms = nullptr;

    VulkanGraphicsApp::cleanup();
}

void Application::render(){

    // Set the position of the top vertex 
    if(glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        glm::vec2 mousePos = getMousePos();
        // Modify the contents of the vertex buffer on the CPU side
        mGeometry->getVertices()[1].pos = glm::vec3(mousePos, 0.0);

        // Upload the new contents to the vertex buffer memory on the GPU side
        mGeometry->updateDevice();

        // Tell the GPU to use the updated vertex buffer
        VulkanGraphicsApp::setVertexBuffer(mGeometry->getBuffer(), mGeometry->vertexCount());
    }

    VkExtent2D frameDimensions = getFramebufferSize();

    // Set the value of our uniform variable ortho matrix
    // Mind the braces, this is a struct initialization
    mWorldUniforms->pushUniformData(WorldInfo{getOrthographicProjection(frameDimensions)});

    // Tell the GPU to render a frame. 
    VulkanGraphicsApp::render();
}

/// Initialize triangle geometry and tell the GPU to draw it. 
void Application::initGeometry(){
    //lab: add two other triangles (see lab write up)
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

    // Create a new vertex buffer on the GPU using the given geometry 
    mGeometry = std::make_shared<SimpleVertexBuffer>(triangleVerts, mDeviceBundle);

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
    // The above description is the Vulkan C++ code that matches up with the GLSL code:
    // layout(location = 0) in vec4 vertPos;
    // layout(location = 1) in vec4 vertCol;

    // Send this description to the GPU so that it knows how to interpret our vertex buffer 
    VulkanGraphicsApp::setVertexInput(vtxInput.getBindingDescription(), vtxInput.getAttributeDescriptions());
}

/// Initialize our shaders
void Application::initShaders(){

    // Load the compiled shader code from disk. 
    VkShaderModule vertShader = vkutils::load_shader_module(mDeviceBundle.logicalDevice.handle(), STRIFY(SHADER_DIR) "/standard.vert.spv");
    VkShaderModule fragShader = vkutils::load_shader_module(mDeviceBundle.logicalDevice.handle(), STRIFY(SHADER_DIR) "/vertexColor.frag.spv");
    
    assert(vertShader != VK_NULL_HANDLE);
    assert(fragShader != VK_NULL_HANDLE);

    VulkanGraphicsApp::setVertexShader("standard.vert", vertShader);
    VulkanGraphicsApp::setFragmentShader("vertexColor.frag", fragShader);
}

/// Initialize uniform data and bind them. 
void Application::initUniforms(){
    // Create uniform data instance
    mWorldUniforms = UniformTransformData::create();
    
    // Add uniform data binding point 0 in our shaders
    // binding point 0 a.k.a `layout(binding = 0)` 
    VulkanGraphicsApp::addUniform(0, mWorldUniforms);
}

/// Compute orthographic projection matrix using viewport dimensions
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
        bottom *= aspect;
    }

    return(glm::ortho(left, right, bottom, top));
}
