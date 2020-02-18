#include "load_obj.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <fstream>
#include <algorithm>

static void process_obj_contents(const tinyobj::attrib_t& attributes, const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, ObjMultiShapeGeometry& ivGeoOut);

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

    // Create vertex array to correct size
    size_t totalVerts = attributes.vertices.size()/3;
    std::vector<ObjVertex> objVertices(totalVerts);

    // Copy vertex positions into array. 
    auto posItr = attributes.vertices.begin();
    auto copyPos = [&](ObjVertex& aVert){
        aVert.position = std::move(glm::vec3(posItr[0], posItr[1], posItr[2]));
        std::advance(posItr, 3);
    };
    std::for_each(objVertices.begin(), objVertices.end(), copyPos);


    
    for(const tinyobj::shape_t& shape : shapes){

        std::vector<ObjMultiShapeGeometry::index_t> outputIndices;
        outputIndices.reserve(shape.mesh.indices.size()); 

        auto indexIter = shape.mesh.indices.begin();
        for(uint8_t numFaces : shape.mesh.num_face_vertices){
            assert(numFaces == 3); // We only know how to work with triangles
            for(int i = 0; i < 3; ++i){
                const tinyobj::index_t& indexBundle = *indexIter++;
                outputIndices.emplace_back(indexBundle.vertex_index);

                // If it exists, copy normal data into the vertex array.
                if(hasNormals && indexBundle.normal_index >= 0) 
                    objVertices[indexBundle.vertex_index].normal = std::move(ptr_to_vec3(&attributes.normals[3*indexBundle.normal_index]));

                // If it exists, copy texture coordinate data into the vertex array.
                if(hasCoords && indexBundle.texcoord_index >= 0) 
                    objVertices[indexBundle.vertex_index].texCoord = std::move(ptr_to_vec2(&attributes.texcoords[2*indexBundle.texcoord_index]));
            }
        }

        // Add shape to MultiShapeGeometry output object
        ivGeoOut.addShape(outputIndices);
    }

    ivGeoOut.setVertices(objVertices);

    // Done
}