#include "Application.h"

    
void Application::init(){
    BasicVulkanApp::init();
}

void Application::run(){
    while(!glfwWindowShouldClose(mWindow)){
        
        glfwPollEvents();
    }
}

void Application::cleanup(){
    BasicVulkanApp::cleanup();
}
