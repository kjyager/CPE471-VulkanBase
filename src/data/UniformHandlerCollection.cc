#include "UniformHandling.h"
#include <limits>

void UniformHandlerCollection::freeAllAndClear() {
    for(std::pair<uint32_t, UniformHandlerPtr> value_pair : *this){
        value_pair.second->free();
    }
    derived_map_t::clear();
}

uint32_t UniformHandlerCollection::getTotalUniformBufferCount() const {
    uint32_t total = 0;
    for(const std::pair<uint32_t, UniformHandlerPtr>& value_pair : *this){
        total += value_pair.second->getBufferCount();
    }
    return(total);
}

std::vector<VkDescriptorSetLayout> UniformHandlerCollection::unrollDescriptorLayouts() const {
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(getTotalUniformBufferCount());
    size_t bufferCount = getTotalUniformBufferCount() / this->size();

    for(size_t i = 0; i < bufferCount; ++i){
        for(const std::pair<uint32_t, UniformHandlerPtr>& value_pair : *this){
            assert(value_pair.second->getBufferCount() == bufferCount);
            layouts.emplace_back(value_pair.second->getRepresentativeDescriptorSetLayout());
        }
    }

    assert(layouts.size() == getTotalUniformBufferCount());
    return(layouts);
}

std::vector<VkDescriptorBufferInfo> UniformHandlerCollection::unrollDescriptorBufferInfo() const {
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(getTotalUniformBufferCount());
    size_t bufferCount = getTotalUniformBufferCount() / this->size();

    for(size_t i = 0; i < bufferCount; ++i){
        for(const std::pair<uint32_t, UniformHandlerPtr>& value_pair : *this){
            assert(value_pair.second->getBufferCount() == bufferCount);
            bufferInfos.emplace_back(VkDescriptorBufferInfo{
                /* buffer = */ value_pair.second->getBufferHandle(i),
                /* offset = */ value_pair.second->getRepresentativeOffset(),
                /* range  = */ value_pair.second->getRepresentativeRange()
            });
        }
    }

    assert(bufferInfos.size() == getTotalUniformBufferCount());
    return(bufferInfos);
}

std::vector<UniformHandlerCollection::ExtraInfo> UniformHandlerCollection::unrollExtraInfo() const {
    std::vector<ExtraInfo> extraInfo;
    extraInfo.reserve(getTotalUniformBufferCount());
    size_t bufferCount = getTotalUniformBufferCount() / this->size();

    for(size_t i = 0; i < bufferCount; ++i){
        for(const std::pair<uint32_t, UniformHandlerPtr>& value_pair : *this){
            assert(value_pair.second->getBufferCount() == bufferCount);
            extraInfo.emplace_back(ExtraInfo{
                value_pair.second->getRepresentativeBinding()
            });
        }
    }

    assert(extraInfo.size() == getTotalUniformBufferCount());
    return(extraInfo);
}
