#include "catch.hpp"
#include "data/UniformBuffer.h"
#include "VulkanSetupBaseApp.h"
#include <glm/glm.hpp>

class VulkanDummyApp : public VulkanSetupBaseApp
{
 public:
    using VulkanSetupBaseApp::mWindow;
    using VulkanSetupBaseApp::mVkInstance;
    using VulkanSetupBaseApp::mPhysDevice;
    using VulkanSetupBaseApp::mLogicalDevice;

};

TEST_CASE("UniformBuffer Tests"){
    using namespace glm;

    SECTION("Construction and Cleanup"){
        VulkanDummyApp dummy;
        dummy.init();

        //TODO: Rewrite these tests. The structure was overhauled.
    }
}