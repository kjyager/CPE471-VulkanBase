#include "catch.hpp"

// Allow unit tests to inspect protected members
#define UNIFORM_BUFFER_UNIT_TESTING

#include "data/UniformBuffer.h"
#include "data/MultiInstanceUniformBuffer.h"
#include <memory>
#include "application/VulkanAppInterface.h"
#include "application/VulkanSetupCore.h"
#include "vkutils/VmaHost.h"

// #define DUMP_BUFFERS

struct DummyVulkanApp : public VulkanAppInterface{
    DummyVulkanApp(){
        mCoreProvider = std::make_shared<VulkanSetupCore>();
        mCoreProvider->linkHostApp(this);
        mCoreProvider->initVulkan();
    }

    virtual const VkApplicationInfo& getAppInfo() const override {
        const static VkApplicationInfo sInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Dummy", 0, "Dummy", 0, VULKAN_BASE_VK_API_VERSION};
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
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);

        size_t alignSize = devBundle.physicalDevice.mProperties.limits.minUniformBufferOffsetAlignment;
        REQUIRE(buffer.getInstanceDataSize() == sizeof(TestStructA) + sizeof(TestStructB));
        REQUIRE(buffer.getPaddedInstanceDataSize() == layoutSet.getTotalPaddedSize(alignSize));
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


        MultiInstanceUniformBuffer buffer(core->getPrimaryDeviceBundle(), layoutSet, 2);
        UniformDataInterfaceSet plainInterfaces = buffer.getInstanceDataInterfaces(0U);

        UniformRawDataPtr raw1 = std::dynamic_pointer_cast<UniformRawData>(plainInterfaces[0]);
        UniformRawDataPtr raw2 = std::dynamic_pointer_cast<UniformRawData>(plainInterfaces[1]);
        REQUIRE(raw1 != nullptr);
        REQUIRE(raw2 != nullptr);
        REQUIRE(raw1->getDataSize() == layoutA->getDataSize());
        REQUIRE(raw2->getDataSize() == layoutB->getDataSize());

        reinterpret_cast<uint32_t*>(raw1->getData())[0] = 0xB0BA;
        reinterpret_cast<uint32_t*>(raw1->getData() + offsetof(TestStructA, b))[0] = 0xBEEF;
        reinterpret_cast<float*>(raw2->getData() + offsetof(TestStructB, b))[0] = 99.99f;

        REQUIRE(buffer.getDeviceSyncState() == DEVICE_OUT_OF_SYNC);
        buffer.updateDevice();
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);

        std::shared_ptr<UniformStructData<TestStructA>> structInterfaceA = UniformStructData<TestStructA>::create();
        structInterfaceA->getStruct().a = 22;
        std::shared_ptr<UniformStructData<TestStructB>> structInterfaceB = UniformStructData<TestStructB>::create();
        structInterfaceB->getStruct().a[0] = 0x33;

        REQUIRE(buffer.mBoundDataInterfaces.size() == 1);

        UniformDataInterfaceSet structInterfaceSet{
            {0, structInterfaceA},
            {1, structInterfaceB}
        };

        buffer.setInstanceDataInterfaces(1, structInterfaceSet);
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_OUT_OF_SYNC);
        REQUIRE(buffer.isBoundDataDirty() == true);
        REQUIRE(buffer.mBoundDataInterfaces.size() == 2);

        UniformStructData<TestStructA>::ptr_t recast = std::dynamic_pointer_cast<UniformStructData<TestStructA>>(buffer.mBoundDataInterfaces[1][0]);
        REQUIRE(recast != nullptr);

        buffer.updateDevice();

        buffer.pushBackInstance(structInterfaceSet);
        REQUIRE(buffer.mBoundDataInterfaces.size() == 3);
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_OUT_OF_SYNC);
        structInterfaceB->getStruct().a[1] = 0x42; 
        REQUIRE(buffer.isBoundDataDirty() == true);
        buffer.updateDevice();
        REQUIRE(buffer.isBoundDataDirty() == false);
        REQUIRE(buffer.getDeviceSyncState() == DEVICE_IN_SYNC);

        VmaAllocator allocator = VmaHost::getAllocator(buffer.mCurrentDevice);
        void* rawMapPtr = nullptr;
        REQUIRE(vmaMapMemory(allocator, buffer.mBufferAllocation, &rawMapPtr) == VK_SUCCESS);
        {
            #ifdef DUMP_BUFFERS
            FILE* dumpBuffer = fopen("MIUB_interfacing_dump.bin", "wb");
            fwrite(rawMapPtr, 1, buffer.mAllocInfo.size, dumpBuffer);
            fclose(dumpBuffer);
            #endif

            const uint8_t* data = reinterpret_cast<const uint8_t*>(rawMapPtr);

            // Modifications by 0th instance
            const uint32_t* p0_A_a = reinterpret_cast<const uint32_t*>(data + buffer.getBoundDataOffset(0));
            const uint32_t* p0_A_b = reinterpret_cast<const uint32_t*>(data + buffer.getBoundDataOffset(0) + offsetof(TestStructA, b));
            const float* p0_B_b0 = reinterpret_cast<const float*>(data + buffer.getBoundDataOffset(1) + offsetof(TestStructB, b));
            CHECK(*p0_A_a == 0XB0BA);
            CHECK(*p0_A_b == 0XBEEF);
            CHECK(*p0_B_b0 == 99.99f);

            // Modifications on 1st instance
            const int* p1_A_a = reinterpret_cast<const int*>(data + buffer.mPaddedBlockSize*1 + buffer.getBoundDataOffset(0));
            const uint8_t* p1_B_a0 = data + buffer.mPaddedBlockSize*1 + buffer.getBoundDataOffset(1) + offsetof(TestStructB, a[0]);
            const uint8_t* p1_B_a1 = data + buffer.mPaddedBlockSize*1 + buffer.getBoundDataOffset(1) + offsetof(TestStructB, a[1]);
            CHECK(*p1_A_a == 22);
            CHECK(*p1_B_a0 == 0x33);
            CHECK(*p1_B_a1 == 0x42);

            // Modifications on 2nd instance
            const int* p2_A_a = reinterpret_cast<const int*>(data + buffer.mPaddedBlockSize*2 + buffer.getBoundDataOffset(0));
            const uint8_t* p2_B_a0 = data + buffer.mPaddedBlockSize*2 + buffer.getBoundDataOffset(1) + offsetof(TestStructB, a[0]);
            const uint8_t* p2_B_a1 = data + buffer.mPaddedBlockSize*2 + buffer.getBoundDataOffset(1) + offsetof(TestStructB, a[1]);
            CHECK(*p2_A_a == 22);
            CHECK(*p2_B_a0 == 0x33);
            CHECK(*p2_B_a1 == 0x42);
        }
        vmaUnmapMemory(allocator, buffer.mBufferAllocation);
        rawMapPtr = nullptr;

        CHECK(buffer.getInstanceCount() == 3);
        buffer.freeAndReset();
    }

    core->cleanup();
}