#ifndef VULKAN_LOAD_OBJ_H_
#define VULKAN_LOAD_OBJ_H_
#include "data/VertexGeometry.h"
#include "data/VertexInput.h"
#include <glm/glm.hpp>
#include <string>
#include <istream>
#include <exception>

class ObjFileException : public std::exception{
 public:
    ObjFileException(const std::string& path) : filepath(path), whatstr("Failed to open OBJ file '" + filepath + "'") {}
    virtual const char* what() const noexcept override {return(whatstr.c_str());}

    const std::string filepath;
 protected:
    const std::string whatstr;
};

class TinyObjFailureException : public std::exception{
 public:
    TinyObjFailureException(const std::string& errstr) : errstr(errstr), whatstr("TinyObj failed to load: '" + errstr + "'") {}
    virtual const char* what() const noexcept override {return(whatstr.c_str());}

    const std::string errstr;
 protected:
    const std::string whatstr;
};

struct ObjVertex{
    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 normal {0.0f, 0.0f, 0.0f};
    glm::vec2 texCoord {0.0f, 0.0f};
};

using ObjMultiShapeGeometry = MultiShapeGeometry<ObjVertex, uint32_t>;
using ObjVertexInput = VertexInputTemplate<ObjVertex>;

const static ObjVertexInput sObjVertexInput(
   0, // Binding point 
   { // Vertex input attributes
      VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjVertex, position)},
      VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ObjVertex, normal)},
      VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ObjVertex, texCoord)}
   }
);

ObjMultiShapeGeometry load_obj_to_vulkan(const VulkanDeviceBundle& aDeviceBundle, const std::string& aObjPath);
ObjMultiShapeGeometry load_obj_to_vulkan(const VulkanDeviceBundle& aDeviceBundle, std::istream& aObjContents);

#endif 