cmake_minimum_required(VERSION 3.10)
include("${CMAKE_SOURCE_DIR}/FindGfxLibs.cmake")
include("${CMAKE_SOURCE_DIR}/VkShaderCompile.cmake")

# Name of the project
project(VulkanOBJ)

# Enforce use of C++14
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_STANDARD 14)

set(VK_ALLOC_LIB_TARGET "vk_mem_alloc")
add_library(${VK_ALLOC_LIB_TARGET} STATIC "${PROJECT_SOURCE_DIR}/ext/vk_mem_alloc.h" "${PROJECT_SOURCE_DIR}/ext/vma_impl.cc")
setupVulkan(${VK_ALLOC_LIB_TARGET})

# Helper function which sets up the correct library linking 
# and compile settings on the builed target 'target' 
function(setBuildProperties target)
    if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/ext")
        target_include_directories(${target} PUBLIC "${PROJECT_SOURCE_DIR}/ext")
    endif()

    linkGLM(${target})
    setupVulkan(${target})
    linkGLFW3(${target})

    # Link vulkan memory allocator library 
    target_link_libraries(${target} ${VK_ALLOC_LIB_TARGET})

    if(NOT WIN32)
      message(STATUS "Adding GCC style compiler flags")
      add_compile_options("-Wall")
      add_compile_options("-Wno-unknown-pragmas")
      add_compile_options("-pedantic")
      add_compile_options("-Werror=return-type")
    else()
      message(STATUS "Adding MSVC compiler flags")
      add_compile_options("/wd4267")
      add_compile_options("/wd4250")
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
      message(STATUS "Adding Apple Clang compiler flags")
      add_compile_options("-stdlib=libc++")
    endif()

endfunction(setBuildProperties)



file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cc" "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.h")

# Add the executable for the project
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/src")

setBuildProperties(${CMAKE_PROJECT_NAME})

# Setup a target to build all glsl shaders
set(SHADER_COMPILE_TARGET "${CMAKE_PROJECT_NAME}.compile_shaders")
addGlslShaderDirectory(${SHADER_COMPILE_TARGET} "${CMAKE_SOURCE_DIR}/shaders")
add_dependencies(${CMAKE_PROJECT_NAME} ${SHADER_COMPILE_TARGET})

# Add preprocessor variables containing the path to the asset and shader directories to the build.
# Students should use these variables when accessing shaders and external resources instead of hardcoding
# or passing the path manually. 
# When building in release mode, the paths are changed so that it is exepcted that the directorires are alongside the executable. 
if(CMAKE_BUILD_TYPE MATCHES Release)
    set(ASSET_DIR "assets/")
    set(SHADER_DIR "${SHADER_BINARY_DIR}")
else()
    set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets/")
    set(SHADER_DIR "${SHADER_BINARY_DIR}")
endif()
add_definitions("-DASSET_DIR=${ASSET_DIR}" "-DSHADER_DIR=${SHADER_DIR}")

find_program(CLANG_TIDY "run-clang-tidy")
if(NOT CLANG_TIDY STREQUAL "CLANG_TIDY-NOTFOUND")
    set(LINTING_TARGET_NAME "clang-tidy")
    add_custom_target(${LINTING_TARGET_NAME} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" COMMAND ${CLANG_TIDY} "-checks='misc-*unused*, readability-misleading-indentation'" DEPENDS ${SOURCES} ${HEADERS})
endif()

# Add unit tests 
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
