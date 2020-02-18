cmake_minimum_required(VERSION 3.4)

# Name of the project
project(VulkanBase)

# Enforce use of C++14
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_STANDARD 14)

# Check for extensions directory as possible location for libraries or other external code
if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/ext")
  set(USING_EXT_DIR TRUE)
  set(EXT_DIR "${CMAKE_SOURCE_DIR}/ext")
endif()

# Get the GLM environment variable. Since GLM is a header-only library, we
# just need to add it to the include directory.
set(GLM_INCLUDE_DIR "$ENV{GLM_INCLUDE_DIR}")
if(NOT GLM_INCLUDE_DIR)
  if(IS_DIRECTORY "${EXT_DIR}/glm")
    set(GLM_INCLUDE_DIR "${EXT_DIR}/glm")
  else()
    MESSAGE(FATAL_ERROR "Please point the environment variable GLM_INCLUDE_DIR to the include directory of your GLM installation.")
  endif()
endif()

# Add GLFW
# Get the GLFW environment variable.
# There should be a CMakeLists.txt in the specified directory.
set(GLFW_DIR "$ENV{GLFW_DIR}")
if(GLFW_DIR)
  message(STATUS "GLFW environment variable found")

  if(NOT EXISTS "${GLFW_DIR}/CMakeLists.txt" AND WIN32)
    FILE(GLOB GLFW_VC_LIB_DIRS "${GLFW_DIR}/lib-vc*")
    if(NOT GLFW_VC_LIB_DIRS)
      message(FATAL_ERROR "GLFW_DIR contains neither a CMakeLists.txt nor pre-compiled libraries for visual studio")
    endif()

    set(GLFW_INCLUDE_DIRS "${GLFW_DIR}/include/")

    function(addMSVCPreCompiled version)
      if(NOT EXISTS "${GLFW_DIR}/lib-vc${version}/glfw3.lib")
        message(FATAL_ERROR "Missing required visual studio pre-compiled library!")
      endif()
      set(GLFW_LIBRARIES "${GLFW_DIR}/lib-vc${version}/glfw3.lib" PARENT_SCOPE)
    endfunction()

    if(MSVC_VERSION GREATER_EQUAL 1920)
      addMSVCPreCompiled("2019")
    elseif(MSVC_VERSION GREATER_EQUAL 1910)
      addMSVCPreCompiled("2017")
    elseif(MSVC_VERSION GREATER_EQUAL 1900)
      addMSVCPreCompiled("2015")
    elseif(MSVC_VERSION GREATER_EQUAL 1800)
      addMSVCPreCompiled("2013")
    elseif(MSVC_VERSION GREATER_EQUAL 1700)
      addMSVCPreCompiled("2012")
    elseif(MSVC_VERSION LESS 1700)
      message(FATAL_ERROR "Visual Studio version is less than minimum (VS 2012)")
    endif()
    
    message(STATUS "Set GLFW_LIBRARIES: ${GLFW_LIBRARIES}")

  elseif(NOT EXISTS "${GLFW_DIR}/CMakeLists.txt" AND APPLE)
    message("GLFW_DIR exists but contains no CMakeLists.txt, trying to add static library")
    include_directories(${GLFW_DIR})
    set(GLFW_LIBRARIES glfw)

  else()
    option(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
    option(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
    option(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)
    if(CMAKE_BUILD_TYPE MATCHES Release)
      add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/release)
    else()
      add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/debug)
    endif()
    if(NOT TARGET glfw)
      message(FATAL_ERROR "glfw library target does not exist despite finding GLFW CMakeLists.txt!")
    endif()
    set(GLFW_LIBRARIES glfw)
  endif()
endif()


# Find the Vulkan package, and error if not found
find_package(Vulkan REQUIRED)

if(NOT VULKAN_SDK AND NOT DEFINED ENV{VULKAN_SDK})
  message(WARNING "Could not find path of the Vulkan SDK. CMake may be unable to find the glsl compiler and fail configuration.\nPlease ensure a 'VULKAN_SDK' environment variable is defined and points to the Vulkan SDK.")
endif()

# Try to find the glsl compiler program. Typically this is included with the SDK. 
find_program(GLSL_COMPILER "glslc" PATHS "$ENV{VULKAN_SDK}/bin/")
if(${GLSL_COMPILER} STREQUAL "GLSL_COMPILER-NOTFOUND")
  message(FATAL_ERROR "Fatal: Unable to find glsl compiler 'glslc'")
else()
  message(STATUS "Found glslc: " ${GLSL_COMPILER})
endif()

# Function to setup libraries and includes on the given build target 
function(BuildProperties TargetName)

  target_include_directories(${TargetName} PUBLIC ${GLM_INCLUDE_DIR})

  if(GLFW_INCLUDE_DIRS)
    target_include_directories(${TargetName} PUBLIC ${GLFW_INCLUDE_DIRS})
  else()
    target_include_directories(${TargetName} PUBLIC ${GLFW_DIR}/include)
  endif()
  
  if(USING_EXT_DIR)
    target_include_directories(${TargetName} PUBLIC "${PROJECT_SOURCE_DIR}/ext")
  endif()
  
  target_link_libraries(${TargetName} ${GLFW_LIBRARIES})

  target_include_directories(${TargetName} PUBLIC ${Vulkan_INCLUDE_DIR})

  if(NOT APPLE)
    target_link_libraries(${TargetName} ${Vulkan_LIBRARY})
  endif()

  if(APPLE)
    target_link_libraries(${TargetName} Vulkan::Vulkan)
  endif()

endfunction(BuildProperties)


file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cc" "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.h")

# Get list of shader source files. Extensions are selected from the expected extensions of the glsl compiler. 
file(GLOB GLSL "shaders/*.vert" "shaders/*.frag" "shaders/*.tesc" "shaders/*.tese" "shaders/*.geom" "shaders/*.comp")
# Special extension for include only glsl files
file(GLOB GLSL_INL "shaders/*.glinl")

if(NOT WIN32)
  message(STATUS "Adding GCC style compiler flags")
  add_compile_options("-Wall")
  add_compile_options("-pedantic")
  add_compile_options("-Werror=return-type")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  message(STATUS "Adding Apple Clang compiler flags")
  add_compile_options("-stdlib=libc++")
endif()

# Add the executable for the project
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/src")

# Setup custom target that creates the output directory where compiled shader code is output
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
  # Create new target for with a name that matches the source file
  get_filename_component(glsl_basename "${glsl_source}" NAME_WE)
  get_filename_component(glsl_extension "${glsl_source}" EXT)
  set(INDIVIDUAL_SHADER_TARGET "${SHADERCOMP_TARGET}.${glsl_basename}")

  # Add onto the target the compile command which is used to compile this glsl source file. The command changes slightly by build type. 
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

  # Make this single glsl file compile target a dependency of the larger compile shaders target. 
  add_dependencies(${SHADERCOMP_TARGET} ${INDIVIDUAL_SHADER_TARGET})
endforeach(glsl_source)

# Make shader compilation a dependency of the project
add_dependencies(${CMAKE_PROJECT_NAME} ${SHADERCOMP_TARGET})

BuildProperties(${CMAKE_PROJECT_NAME})

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

add_subdirectory("${PROJECT_SOURCE_DIR}/tests")