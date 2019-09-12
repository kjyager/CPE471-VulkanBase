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

find_program(GLSL_COMPILER "glslc" PATHS "$ENV{VULKAN_SDK}/bin/")
if(${GLSL_COMPILER} STREQUAL "GLSL_COMPILER-NOTFOUND")
  message(FATAL_ERROR "Fatal: Unable to find glsl compiler 'glslc'.")
else()
  message("Found glslc: " ${GLSL_COMPILER})
endif()

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
file(GLOB GLSL "shaders/*.vert" "shaders/*.frag" "shaders/*.tesc" "shaders/*.tese" "shaders/*.geom" "shaders/*.comp")
file(GLOB GLSL_INL "shaders/*.glinl")

set(SOURCES ${SOURCES_UNFILTERED})
set(HEADERS ${HEADERS_UNFILTERED})


# Add the executable for the project
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/src")

# Setup custom target to create the output directory where compiled shader code will go
set(SHADERCOMP_SETUP_TARGET "${CMAKE_PROJECT_NAME}.compile_shaders_setup")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
add_custom_target(${SHADERCOMP_SETUP_TARGET})
add_custom_command(TARGET ${SHADERCOMP_SETUP_TARGET} 
  POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
)

# Create target for compiling all GLSL files.
set(SHADERCOMP_TARGET "${CMAKE_PROJECT_NAME}.compile_shaders")
add_custom_target(${SHADERCOMP_TARGET})

# Loop over GLSL source files and create a compile target for each
foreach(glsl_source ${GLSL})
  get_filename_component(glsl_basename "${glsl_source}" NAME_WE)
  get_filename_component(glsl_extension "${glsl_source}" EXT)
  set(INDIVIDUAL_SHADER_TARGET "${SHADERCOMP_TARGET}.${glsl_basename}")

  # The compile command used to compile this glsl source file. Changes based on build type. 
  if(CMAKE_BUILD_TYPE MATCHES Release)
    add_custom_command(OUTPUT "${glsl_basename}${glsl_extension}.spv"
      COMMAND "${GLSL_COMPILER}" "--target-env=vulkan1.1" "-x" "glsl" "-c" "-O" "${glsl_source}"
      WORKING_DIRECTORY "${SHADER_BINARY_DIR}"
    )
  else()
    add_custom_command(OUTPUT "${glsl_basename}${glsl_extension}.spv"
      COMMAND "${GLSL_COMPILER}" "--target-env=vulkan1.1" "-x" "glsl" "-c" "-g" "-O0" "${glsl_source}"
      WORKING_DIRECTORY "${SHADER_BINARY_DIR}"
    )
  endif()
  # Create the single glsl file compile target with the compiled SPIR-V as it's dependency.
  # This has the effect of linking the prior command to this target so that it will run when the target is built.
  add_custom_target(${INDIVIDUAL_SHADER_TARGET} DEPENDS "${glsl_basename}${glsl_extension}.spv" ${SHADERCOMP_SETUP_TARGET} )

  # Make this single glsl file compile target a dependency of the larger compile shadrs target
  add_dependencies(${SHADERCOMP_TARGET} ${INDIVIDUAL_SHADER_TARGET})
endforeach(glsl_source)

# Make shader compilation a dependency of the project
add_dependencies(${CMAKE_PROJECT_NAME} ${SHADERCOMP_TARGET})


BuildProperties(${CMAKE_PROJECT_NAME})

if(CMAKE_BUILD_TYPE MATCHES Release)
  set(ASSET_DIR "assets/")
  set(SHADER_DIR "shaders/")
else()
  set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets/")
  set(SHADER_DIR "shaders/") # No change since shaders are compiled with the project and will be stored relative to the executable
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  # Pass 
endif()

add_definitions("-DASSET_DIR=${ASSET_DIR}" "-DSHADER_DIR=${SHADER_DIR}")