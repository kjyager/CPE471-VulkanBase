#include "catch.hpp"

// Allow unit tests to inspect protected members
#define UNIFORM_BUFFER_UNIT_TESTING

#include "data/UniformBuffer.h"
#include "data/MultiInstanceUniformBuffer.h"
#include <memory>
#include "application/VulkanAppInterface.h"
#include "application/VulkanSetupCore.h"

struct DummyVulkanApp : public VulkanAppInterface{
    DummyVulkanApp(){
        mCoreProvider = std::make_shared<VulkanSetupCore>();
        mCoreProvider->linkHostApp(this);
        mCoreProvider->initVulkan();
    }

    virtual const VkApplicationInfo& getAppInfo() const override {
        const static VkApplicationInfo sInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Dummy", 0, "Dummy", 0, VK_API_VERSION_1_1};
        return(sInfo);
    }

    virtual const std::vector<std::string>& getRequestedValidationLayers() const override {
        const static std::vector<std::string> sLayers{"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor"};
        return(sLayers);
    };
    
    std::shared_ptr<VulkanSetupCore> mCoreProvider = nullptr;
};

struct TestStructA{
    alignas(16) int a;
    alignas(16) int b;
};
struct TestStructB{
    alignas(16) uint8_t a[3];
    alignas(16) float b[34];
};


TEST_CASE("Uniform Buffer Utilities"){

    SECTION("Uniform Raw Data"){
        using UniformRawDataPtr = std::shared_ptr<UniformRawData>;

        const static int four = 4;
        const static int fourtyTwo = 42;
        const uint8_t* data1 = reinterpret_cast<const uint8_t*>(&four);
        const uint8_t* data2 = reinterpret_cast<const uint8_t*>(&fourtyTwo);

        UniformRawDataPtr ptr1 = UniformRawData::create<32>();
        UniformRawDataPtr ptr2 = UniformRawData::create(128);
        UniformRawDataPtr ptr3 = UniformRawData::create<sizeof(int)>(data1);
        UniformRawDataPtr ptr4 = UniformRawData::create(sizeof(int), data2);

        REQUIRE(ptr1->getDataSize() == 32);
        REQUIRE(ptr2->getDataSize() == 128);
        REQUIRE(ptr3->getDataSize() == sizeof(int));
        REQUIRE(ptr4->getDataSize() == sizeof(int));

        REQUIRE(ptr1->getPaddedDataSize(128) == 128);
        REQUIRE(ptr2->getPaddedDataSize(100) == 200);
        REQUIRE(ptr3->getPaddedDataSize(32) == 32);
        REQUIRE(ptr4->getPaddedDataSize(1) == sizeof(int));

        int value1 = *reinterpret_cast<const int*>(ptr3->getData());
        int value2 = *reinterpret_cast<const int*>(ptr4->getData());
        REQUIRE(value1 == four);
        REQUIRE(value2 == fourtyTwo);
    }

    SECTION("Uniform Struct Data"){
        using UniformDataA = UniformStructData<TestStructA>;
        using UniformDataB = UniformStructData<TestStructB>;

        std::shared_ptr<UniformDataA> ptr1 = UniformDataA::create();
        std::shared_ptr<UniformDataB> ptr2 = UniformDataB::create();

        REQUIRE(ptr1->getPaddedDataSize(128) == 128);
        REQUIRE(ptr1->getDataSize() == 32);
        REQUIRE(ptr2->getPaddedDataSize(128) == 256);
        REQUIRE(ptr2->getDataSize() == sizeof(TestStructB));

        UniformDataLayoutSet layoutSet = {
            {1, ptr1},
            {5, ptr2}
        };

        REQUIRE(layoutSet.getTotalPaddedSize(128) == 384);
        REQUIRE(layoutSet.getBoundDataOffset(1, 128) == 0);
        REQUIRE(layoutSet.getBoundDataOffset(5, 128) == 128);

    }

}

TEST_CASE("Multi Instance Uniform Buffer Tests"){

    DummyVulkanApp app;
    std::shared_ptr<VulkanSetupCore> core = app.mCoreProvider;
    const VulkanDeviceBundle& devBundle = core->getPrimaryDeviceBundle();


    SECTION("Construction Test"){
        UniformDataLayoutPtr layoutA = UniformStructDataLayout<TestStructA>::create();
        UniformDataLayoutPtr layoutB = UniformStructDataLayout<TestStructB>::create();
        UniformDataLayoutSet layoutSet {
            {0, layoutA},
            {1, layoutB}
        };


        MultiInstanceUniformBuffer buffer(core->getPrimaryDeviceBundle(), layoutSet, 3);
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_OUT_OF_SYNC);
        buffer.updateDevice();
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);

        size_t alignSize = devBundle.physicalDevice.mProperties.limits.minUniformBufferOffsetAlignment;
        REQUIRE(buffer.getInstanceDataSize() == layoutSet.getTotalPaddedSize(alignSize));
        REQUIRE(buffer.getBoundDataOffset(0) == layoutSet.getBoundDataOffset(0, alignSize));
        REQUIRE(buffer.getBoundDataOffset(1) == layoutSet.getBoundDataOffset(1, alignSize));

        REQUIRE(buffer.getInstanceCount() == 3);
        REQUIRE(buffer.getCapcity() == 3);

        REQUIRE(buffer.isBoundDataDirty() == false);

        buffer.freeAndReset();
    }

    SECTION("Resizing Test"){
        UniformDataLayoutPtr layoutA = UniformStructDataLayout<TestStructA>::create();
        UniformDataLayoutPtr layoutB = UniformStructDataLayout<TestStructB>::create();
        UniformDataLayoutSet layoutSet {
            {0, layoutA},
            {1, layoutB}
        };


        MultiInstanceUniformBuffer buffer(core->getPrimaryDeviceBundle(), layoutSet, 3);
        REQUIRE(buffer.getInstanceCount() == 3);
        REQUIRE(buffer.getCapcity() == 3);
        buffer.updateDevice();
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);
        buffer.setCapcity(5);
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);
        REQUIRE(buffer.getInstanceCount() == 3);
        REQUIRE(buffer.getCapcity() == 5);

        REQUIRE(buffer.pushBackInstance() == 3);
        REQUIRE(buffer.pushBackInstance() == 4);
        REQUIRE(buffer.getInstanceCount() == 5);
        REQUIRE(buffer.getCapcity() == 5);
        REQUIRE(buffer.pushBackInstance() == 5);
        REQUIRE(buffer.getInstanceCount() == 6);
        REQUIRE(buffer.getCapcity() == 8);

        buffer.freeAndReset();
    }

    SECTION("Interfacing Test"){
        UniformDataLayoutPtr layoutA = UniformStructDataLayout<TestStructA>::create();
        UniformDataLayoutPtr layoutB = UniformStructDataLayout<TestStructB>::create();
        UniformDataLayoutSet layoutSet {
            {0, layoutA},
            {1, layoutB}
        };


        MultiInstanceUniformBuffer buffer(core->getPrimaryDeviceBundle(), layoutSet, 1);
        UniformDataInterfaceSet plainInterfaces = buffer.getInstanceDataInterfaces(0U);

        UniformRawDataPtr raw1 = std::dynamic_pointer_cast<UniformRawData>(plainInterfaces[0]);
        UniformRawDataPtr raw2 = std::dynamic_pointer_cast<UniformRawData>(plainInterfaces[1]);
        REQUIRE(raw1 != nullptr);
        REQUIRE(raw2 != nullptr);
        REQUIRE(raw1->getDataSize() == layoutA->getDataSize());
        REQUIRE(raw2->getDataSize() == layoutB->getDataSize());

        buffer.freeAndReset();
    }

    core->cleanup();
}