#include "catch.hpp"

#include "data/UniformBuffer.h"
#include <memory>
// #include "application/VulkanAppInterface.h"
// #include "application/VulkanSetupCore.h"

TEST_CASE("Uniform Buffer Tests"){
    struct TestStructA{
        alignas(16) int a;
        alignas(16) int b;
    };
    struct TestStructB{
        alignas(16) uint8_t a[3];
        alignas(16) float b[34];
    };
    SECTION("Small Utilities"){
        using UniformDataA = UniformStructData<TestStructA>;
        using UniformDataB = UniformStructData<TestStructB>;

        std::shared_ptr<UniformDataA> aPtr = UniformDataA::create();
        std::shared_ptr<UniformDataB> bPtr = UniformDataB::create();

        REQUIRE(aPtr->getPaddedDataSize(128) == 128);
        REQUIRE(aPtr->getDataSize() == 32);
        REQUIRE(bPtr->getPaddedDataSize(128) == 256);
        REQUIRE(bPtr->getDataSize() == sizeof(TestStructB));

        UniformDataLayoutSet layoutSet = {
            {1, aPtr},
            {5, bPtr}
        };

        REQUIRE(layoutSet.getTotalPaddedSize(128) == 384);
        REQUIRE(layoutSet.getBoundDataOffset(1, 128) == 0);
        REQUIRE(layoutSet.getBoundDataOffset(5, 128) == 128);

    }
}