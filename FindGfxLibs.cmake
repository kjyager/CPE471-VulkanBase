cmake_minimum_required(VERSION 3.10)
include(CheckIncludeFileCXX)

# Finds Vulkan and the glsl compiler. The variable GLSL_COMPILER is exported
# with the path to the glsl compiler executable.
function(findVulkan)
    # Find the Vulkan package, and error if not found
    find_package(Vulkan REQUIRED)

    if(NOT VULKAN_SDK AND NOT DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK "VULKAN_SDK-NOTFOUND")
    endif()

    # Try to find the glsl compiler program. Typically this is included with the SDK. 
    find_program(GLSL_COMPILER "glslc" PATHS "$ENV{VULKAN_SDK}/bin/")
    if(${GLSL_COMPILER} STREQUAL "GLSL_COMPILER-NOTFOUND")
        message(FATAL_ERROR "Fatal: Unable to find glsl compiler 'glslc'")
        if(VULKAN_SDK STREQUAL "VULKAN_SDK-NOTFOUND")
            message(WARNING "Could not find path of the Vulkan SDK. CMake may be unable to find the glsl compiler and fail configuration.\nPlease ensure a 'VULKAN_SDK' environment variable is defined and points to the Vulkan SDK.")
        endif()
    else()
        message(STATUS "Found glslc: " ${GLSL_COMPILER})
    endif()

    set(GLSL_COMPILER ${GLSL_COMPILER} PARENT_SCOPE)
    set(GLSL_COMPILER ${GLSL_COMPILER} CACHE FILEPATH "Path to glsl compiler (typically somewhere in the Vulkan SDK)")

endfunction(findVulkan)

# Configures 'target' to build against Vulkan.
function(setupVulkan target)

    if(NOT DEFINED Vulkan_INCLUDE_DIR OR NOT DEFINED Vulkan_LIBRARY)
        findVulkan()
    endif()

    target_include_directories(${target} PUBLIC ${Vulkan_INCLUDE_DIR})

    if(NOT APPLE)
        target_link_libraries(${target} ${Vulkan_LIBRARY})
    endif()

    if(APPLE)
        target_link_libraries(${target} Vulkan::Vulkan)
    endif()

endfunction(setupVulkan)

# Find GLFW3 using find_package or environment variable 
# Exports cache varaibles GLFW_LIBRARIES and GLFW_INCLUDE_DIRS which can be
# used for includes and linking. GLFW_INCLUDE_DIRS may be empty, and 
# should indicate that the include files are located in a standard 
# include location. 
function(findGLFW3)

    find_package(glfw3 QUIET)
    set(GLFW_FROM_SOURCE FALSE CACHE INTERNAL "Indicates that GLFW is being built from a source package")

    if(glfw3_FOUND)

        ASSERT_TARGET(glfw)
        # Include paths are added automatically when linking the glfw3 target from find_package()
        set(GLFW_LIBRARIES glfw)
        set(GLFW_INCLUDE_DIRS "")

    elseif(DEFINED ENV{GLFW_DIR})

        set(GLFW_DIR "$ENV{GLFW_DIR}")

        if(NOT EXISTS "${GLFW_DIR}/CMakeLists.txt" AND WIN32)
            _findGLFW3_vsbinary(target) 
        elseif(EXISTS "${GLFW_DIR}/CMakeLists.txt")
            _findGLFW3_sourcepkg(target)
        else()
            message(FATAL_ERROR "GLFW environment variable 'GLFW_DIR' found, but points to a directory which is not a source package containing 'CMakeLists.txt'.")
        endif()

        if(NOT GLFW_LIBRARIES)
            message(FATAL_ERROR "Internal Error! GLFW_LIBRARIES variable did not get set! Contact your TA, this is their fault.")
        endif()

    else()
        message(FATAL_ERROR "glfw3 could not be found through find_package or environment varaible 'GLFW_DIR'! glfw3 must be installed!")
    endif()


    ASSERT_DEFINED(GLFW_LIBRARIES)
    ASSERT_DEFINED(GLFW_INCLUDE_DIRS)
    # Export GLFW_LIBRARIES variable to parent scope 
    set(GLFW_LIBRARIES ${GLFW_LIBRARIES} CACHE PATH "Path to glfw3 library")
    set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS} CACHE PATH "Paht to glfw3 include direcotry")

endfunction(findGLFW3)

# Find and link GLFW3 using find_package or environment variable 
# Exports varaibles GLFW_LIBRARIES and GLFW_INCLUDE_DIRS which can be
# used for includes and linking. GLFW_INCLUDE_DIRS may be empty, and 
# should indicate that the include files are located in a standard 
# include location. 
function(linkGLFW3 target)

    if(NOT DEFINED GLFW_INCLUDE_DIRS OR NOT DEFINED GLFW_LIBRARIES)
        findGLFW3()
    endif()

    if(${GLFW_FROM_SOURCE} AND NOT TARGET ${GLFW_LIBRARIES})
        findGLFW3()
    endif()

    if(NOT GLFW_INCLUDE_DIRS STREQUAL "")
        target_include_directories(${target} PUBLIC ${GLFW_INCLUDE_DIRS})
    endif()
    
    target_link_libraries(${target} ${GLFW_LIBRARIES})

endfunction(linkGLFW3)


# Find and link GLM using find_package or environment variable
# MAY export variable GLM_INCLUDE_DIRS if include directory(s) were 
# found somewhere other than a default include location. 
function(linkGLM target)

    if(NOT DEFINED GLM_INCLUDE_DIRS)
        find_package(glm QUIET)

        if(NOT glm_FOUND)
            set(GLM_INCLUDE_DIRS "$ENV{GLM_INCLUDE_DIR}")
            if(NOT GLM_INCLUDE_DIRS)

                # Attempt to verify that glm headers are already visible to the compiler. 
                CHECK_INCLUDE_FILE_CXX("glm/glm.hpp" GLM_HEADER_FOUND)
                if(NOT GLM_HEADER_FOUND)
                    message(WARNING "glm installation could not be verified. Builds will likely fail!")
                    return()
                endif()
            endif()
        endif()

    endif()

    if(GLM_INCLUDE_DIRS AND TARGET ${target})
        set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIRS} CACHE PATH "Path to glm include directory")
        target_include_directories(${target} PUBLIC "${GLM_INCLUDE_DIRS}")
    endif()
    
endfunction(linkGLM)



# # # # # # # # # # #
# Helper functions  # 
# # # # # # # # # # #

# findGLFW helper function
function(_findGLFW3_vsbinary target)

    FILE(GLOB GLFW_VC_LIB_DIRS "${GLFW_DIR}/lib-vc*")
    
    if(NOT GLFW_VC_LIB_DIRS)
        message(FATAL_ERROR "GLFW_DIR contains neither a CMakeLists.txt nor pre-compiled libraries for visual studio")
    endif()

    function(addMSVCPreCompiled version)
        if(NOT EXISTS "${GLFW_DIR}/lib-vc${version}/glfw3.lib")
            message(STATUS "No pre-compiled library for requested Visual Studio version. Attempting to use older...")
        else()
            set(GLFW_LIBRARIES "${GLFW_DIR}/lib-vc${version}/glfw3.lib" PARENT_SCOPE)
        endif()
    endfunction()

    if(MSVC_VERSION GREATER_EQUAL 1920)
        addMSVCPreCompiled("2019")
    endif()
    if(MSVC_VERSION GREATER_EQUAL 1910)
        addMSVCPreCompiled("2017")
    endif()
    if(MSVC_VERSION GREATER_EQUAL 1900)
        addMSVCPreCompiled("2015")
    endif()
    if(MSVC_VERSION LESS 1900)
        message(FATAL_ERROR "Visual Studio version is less than minimum (VS 2015)")
    endif()

    if(NOT GLFW_LIBRARIES)
        message(FATAL_ERROR "No usable pre-compiled glfw3 library could be found in given directory!")
    endif()

    set(GLFW_LIBRARIES ${GLFW_LIBRARIES} PARENT_SCOPE)
    set(GLFW_INCLUDE_DIRS "${GLFW_DIR}/include" PARENT_SCOPE)
    message(STATUS "VS pre-compiled binary search set GLFW_LIBRARIES: ${GLFW_LIBRARIES}")

endfunction(_findGLFW3_vsbinary)

# findGLFW helper function
function(_findGLFW3_sourcepkg target)

    option(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
    option(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
    option(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)

    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/release)
    else()
        add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/debug)
    endif()

    set(GLFW_LIBRARIES glfw PARENT_SCOPE)
    set(GLFW_INCLUDE_DIRS "${GLFW_DIR}/include" PARENT_SCOPE)

    set(GLFW_FROM_SOURCE TRUE CACHE INTERNAL "Indicates that GLFW is being built from a source package" FORCE)

endfunction(_findGLFW3_sourcepkg)

function(ASSERT_TARGET target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "ASSERT_TARGET failed on ${target}")
    endif()
endfunction(ASSERT_TARGET)

function(ASSERT_DEFINED var)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "ASSERT_DEFINED failed on ${var}")
    endif()
endfunction(ASSERT_DEFINED)