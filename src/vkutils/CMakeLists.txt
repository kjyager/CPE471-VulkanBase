cmake_minimum_required(VERSION 3.10)

set(VKUTILS_LIBRARY_NAME "vkutils")

# Uncomment to force use of c++ 17
# set(CMAKE_CXX_STANDARD_REQUIRED 17)
# set(CMAKE_CXX_STANDARD 17)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find Vulkan Memory Allocator
find_library(VK_MEM_ALLOC_LIB vk_mem_alloc libvkma)
find_path(VK_MEM_ALLOC_INCLUDE_DIR "vk_mem_alloc.h")

if(VK_MEM_ALLOC_LIB STREQUAL "VK_MEM_ALLOC_LIB-NOTFOUND")
    message(FATAL_ERROR "Cannot find Vulkan memory allocator library. Please specify it's location manually in the CMAKE cache!")
endif()

if(VK_MEM_ALLOC_INCLUDE_DIR STREQUAL "VK_MEM_ALLOC_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "Cannot find Vulkan memory allocator library header. Please specify it's location manually in the CMAKE cache!")
endif()

# Gather source files
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/*.cc" "${PROJECT_SOURCE_DIR}/*.c" "${PROJECT_SOURCE_DIR}/*.inl")
file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/*.hpp" "${PROJECT_SOURCE_DIR}/*.h")

# Create library target
add_library(${VKUTILS_LIBRARY_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${VKUTILS_LIBRARY_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})

if(NOT APPLE)
    target_link_libraries(${VKUTILS_LIBRARY_NAME} ${Vulkan_LIBRARY})
endif()

if(APPLE)
    target_link_libraries(${VKUTILS_LIBRARY_NAME} Vulkan::Vulkan)
endif()

target_link_libraries(${VKUTILS_LIBRARY_NAME} ${VK_MEM_ALLOC_LIB})
target_include_directories(${VKUTILS_LIBRARY_NAME} PRIVATE ${VK_MEM_ALLOC_INCLUDE_DIR})