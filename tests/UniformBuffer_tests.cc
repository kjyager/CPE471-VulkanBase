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

        struct TestStruct{
            vec2 grid = vec2(1.0, -1.0);
            vec3 cube = vec3(1.0, -1.0, 42.0);
        };

        VulkanDeviceHandlePair devInfo = {dummy.mLogicalDevice, dummy.mPhysDevice};

        UniformBuffer<TestStruct> ubo(0, devInfo);

        REQUIRE(ubo.getDeviceSyncState() == DEVICE_IN_SYNC);

        REQUIRE(ubo.getDescriptorSetLayout() != VK_NULL_HANDLE);

        REQUIRE(ubo.getBinding() == 0);

        REQUIRE(ubo.getCurrentDevice().device == dummy.mLogicalDevice);

        REQUIRE(ubo.getCurrentDevice().physicalDevice == dummy.mPhysDevice);

        REQUIRE(ubo.getUniformDataConst().grid == vec2(1.0, -1.0));
        REQUIRE(ubo.getUniformDataConst().cube == vec3(1.0, -1.0, 42.0));

        REQUIRE(ubo.getDeviceSyncState() == DEVICE_IN_SYNC);

        ubo.getUniformData().grid = vec2(2.0, 2.0);

        REQUIRE(ubo.getDeviceSyncState() == DEVICE_OUT_OF_SYNC);

        ubo.updateDevice();

        REQUIRE(ubo.getDeviceSyncState() == DEVICE_IN_SYNC);

        REQUIRE(ubo.getUniformDataConst().grid == vec2(2.0, 2.0));

        ubo.freeBuffer();

        REQUIRE(ubo.getBuffer() == VK_NULL_HANDLE);

        dummy.cleanup();
    }
}