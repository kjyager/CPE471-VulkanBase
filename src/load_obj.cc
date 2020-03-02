#include "load_obj.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

static void process_obj_contents(const tinyobj::attrib_t& attributes, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, ObjMultiShapeGeometry& ivGeoOut);

/// TinyObj index type that can be used in a hash table. 
struct index_t : public tinyobj::index_t{
    index_t() = default;
    index_t(const tinyobj::index_t& aOther) : tinyobj::index_t(aOther) {}

    friend bool operator==(const index_t& a, const index_t& b) { return(a.vertex_index == b.vertex_index && a.normal_index == b.normal_index && a.texcoord_index == b.texcoord_index); }
    friend bool operator!=(const index_t& a, const index_t& b) { return(!operator==(a,b)); }
};

template<>
struct std::hash<index_t>{
    size_t operator()(const index_t& aIndexBundle) const noexcept{
        return(
            ((std::hash<int>()(aIndexBundle.vertex_index)
            ^
            (std::hash<int>()(aIndexBundle.normal_index) << 1)) >> 1)
            ^
            (std::hash<int>()(aIndexBundle.texcoord_index) << 1)
        );
    }
};


ObjMultiShapeGeometry load_obj_to_vulkan(const VulkanDeviceBundle& aDeviceBundle, const std::string& aObjPath){
    std::ifstream objFileStream = std::ifstream(aObjPath);
    if(!objFileStream.is_open()){
        perror(aObjPath.c_str());
        throw new ObjFileException(aObjPath);
    }

    return(load_obj_to_vulkan(aDeviceBundle, objFileStream));
}

ObjMultiShapeGeometry load_obj_to_vulkan(const VulkanDeviceBundle& aDeviceBundle, std::istream& aObjContents){
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool success = tinyobj::LoadObj(&attributes, &shapes, &materials, &err, &aObjContents);
    if(!success){
        throw TinyObjFailureException(err);
    }

    ObjMultiShapeGeometry ivGeo(aDeviceBundle);
    process_obj_contents(attributes, shapes, materials, ivGeo);

    return(ivGeo);
}

/// glm doesn't support contruction from pointer, so we'll fake it. 
static glm::vec3 ptr_to_vec3(const float* aData){
    return(glm::vec3(
        aData[0],
        aData[1],
        aData[2]
    ));
}

/// glm doesn't support contruction from pointer, so we'll fake it. 
static glm::vec2 ptr_to_vec2(const float* aData){
    return(glm::vec2(
        aData[0],
        aData[1]
    ));
}

static void process_obj_contents(const tinyobj::attrib_t& attributes, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, ObjMultiShapeGeometry& ivGeoOut){
    // Verify assumptions about obj data
    assert(sizeof(tinyobj::real_t) == sizeof(float));
    assert(attributes.vertices.size() % 3 == 0);
    assert(attributes.normals.size() % 3 == 0);
    assert(attributes.texcoords.size() % 2 == 0);
    assert(attributes.vertices.size() != 0);

    bool hasNormals = attributes.normals.size() != 0;
    bool hasCoords = attributes.texcoords.size() != 0;

    // Count the maximum number of vertices needed for this model
    using tinyobj::shape_t;
    size_t totalIndices = std::accumulate(
        shapes.begin(), shapes.end(), 0,
        [](size_t t, const shape_t& shape){
            return(t + shape.mesh.indices.size());
        }
    );

    // Allocate space for all vertices. 
    std::vector<ObjVertex> objVertices;
    objVertices.reserve(totalIndices/3);

    // Map allows us to avoid duplicating vertices by ignoring combinations of attributes we've already seen. 
    std::unordered_map<index_t, size_t> seenIndices;

    // Loop over shapes in the obj file
    for(const tinyobj::shape_t& shape : shapes){

        std::vector<ObjMultiShapeGeometry::index_t> outputIndices;
        outputIndices.reserve(shape.mesh.indices.size()); 

        // Loop over all faces while also iterating over indexing information
        auto indexIter = shape.mesh.indices.begin();
        for(uint8_t numFaces : shape.mesh.num_face_vertices){

            assert(numFaces == 3); // We only know how to work with triangles

            // Loop over the three vertices of a triangle. 
            for(int i = 0; i < 3; ++i){
                const index_t& indexBundle = *indexIter++;
                auto findExistingVert = seenIndices.find(indexBundle);
                if(findExistingVert != seenIndices.end()){
                    outputIndices.push_back(static_cast<uint32_t>(findExistingVert->second));
                }else{
                    outputIndices.emplace_back(objVertices.size());
                    
                    objVertices.emplace_back(ObjVertex{
                        ptr_to_vec3(&attributes.vertices[indexBundle.vertex_index*3]),
                        {}, {}
                    });

                    // If it exists, copy normal data into the vertex array.
                    if(hasNormals && indexBundle.normal_index >= 0)
                        objVertices.back().normal = std::move(ptr_to_vec3(&attributes.normals[3*indexBundle.normal_index]));

                    // If it exists, copy texture coordinate data into the vertex array.
                    if(hasCoords && indexBundle.texcoord_index >= 0) 
                        objVertices.back().texCoord = std::move(ptr_to_vec2(&attributes.texcoords[2*indexBundle.texcoord_index]));
                }
            }
        }

        // Add shape to MultiShapeGeometry output object
        ivGeoOut.addShape(outputIndices);
    }

    // Add all vertices to output object
    ivGeoOut.setVertices(objVertices);

    // Done
}