#ifndef APPLICATION_H_
#define APPLICATION_H_
#include "BasicVulkanApp.h"

class Application : BasicVulkanApp{
 public:
    
    void init();
    void run();
    void cleanup();
};

#endif