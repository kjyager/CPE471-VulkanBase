cmake_minimum_required(VERSION 3.4)

# Name of the project
project(VulkanIntro)

# Get the GLM environment variable. Since GLM is a header-only library, we
# just need to add it to the include directory.
set(GLM_INCLUDE_DIR "$ENV{GLM_INCLUDE_DIR}")
if(NOT GLM_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Please point the environment variable GLM_INCLUDE_DIR to the include directory of your GLM installation.")
endif()

# Get the GLFW environment variable. There should be a CMakeLists.txt in the 
# specified directory.
set(GLFW_DIR "$ENV{GLFW_DIR}")
if(NOT GLFW_DIR)
  message(FATAL_ERROR "Please point the environment variable GLFW_DIR to the root directory of your GLFW3 installation.")
endif()
option(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
option(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
option(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)
if(CMAKE_BUILD_TYPE MATCHES Release)
  add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/release)
else()
  add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/debug)
endif()

# Find the Vulkan package, and error if not found
find_package(Vulkan REQUIRED)

function(BuildProperties TargetName)

  target_include_directories(${TargetName} PUBLIC ${GLM_INCLUDE_DIR})
  target_include_directories(${TargetName} PUBLIC ${GLFW_DIR}/include)
  target_include_directories(${TargetName} PUBLIC "${PROJECT_SOURCE_DIR}/ext")
  target_link_libraries(${TargetName} glfw ${GLFW_LIBRARIES})
  target_link_libraries(${TargetName} vulkan ${VULKAN_LIB})

endfunction(BuildProperties)


set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message("Enforcing returns in C++ functions")
  add_compile_options("-Werror=return-type")
endif()


file(GLOB_RECURSE SOURCES_UNFILTERED "${PROJECT_SOURCE_DIR}/src/*.cc" "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS_UNFILTERED "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.h")
file(GLOB GLSL "shaders/*.vs" "shaders/*.fs" "shaders/*.cs" "shaders/*.frag" "shaders/*.vert" "shaders/*.gs" "shaders/*.ts")

set(SOURCES ${SOURCES_UNFILTERED})
set(HEADERS ${HEADERS_UNFILTERED})


# Add the executable for the project
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/src")

BuildProperties(${CMAKE_PROJECT_NAME})

if(CMAKE_BUILD_TYPE MATCHES Release)
  set(ASSET_DIR "assets/")
  set(SHADER_DIR "shaders/")
else()
  set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets/")
  set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders/")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  # Pass 
endif()

add_definitions("-DASSET_DIR=${ASSET_DIR}" "-DSHADER_DIR=${SHADER_DIR}")