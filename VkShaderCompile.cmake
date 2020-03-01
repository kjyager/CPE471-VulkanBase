cmake_minimum_required(VERSION 3.10)

function(initVulkanShaderCore)
    set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders" CACHE PATH "Location of glsl shader SPIR-V binaries after compilation")

    if(NOT TARGET ${SHADERCOMP_SETUP_TARGET})
        set(SHADERCOMP_SETUP_TARGET "${CMAKE_PROJECT_NAME}.compile_shaders_setup" CACHE INTERNAL "<INTERNAL>")
        add_custom_target(${SHADERCOMP_SETUP_TARGET})
        add_custom_command(TARGET ${SHADERCOMP_SETUP_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}")
    endif()
endfunction(initVulkanShaderCore)


function(addGlslShaderDirectory targetName directory)
    initVulkanShaderCore()

    # Create target for compiling all GLSL files in directory
    add_custom_target(${targetName})

    file(GLOB GLSL "${directory}/*.vert" "${directory}/*.frag" "${directory}/*.tesc" "${directory}/*.tese" "${directory}/*.geom" "${directory}/*.comp")

    # Loop over GLSL source files and create a compile target for each
    foreach(glslSource ${GLSL})
        get_filename_component(glslBasename "${glslSource}" NAME_WE)
        get_filename_component(glslExtension "${glslSource}" EXT)
        set(INDIVIDUAL_SHADER_TARGET "${targetName}.${glslBasename}")

        addGlslShader(${INDIVIDUAL_SHADER_TARGET} ${glslSource})
        add_dependencies(${INDIVIDUAL_SHADER_TARGET} ${SHADERCOMP_SETUP_TARGET})

        # Make this single glsl file compile target a dependency of the larger compile shaders target. 
        add_dependencies(${targetName} ${INDIVIDUAL_SHADER_TARGET})

    endforeach(glslSource)
    
endfunction(addGlslShaderDirectory)


function(addGlslShader targetName glslSource)
    initVulkanShaderCore()
    # Create new target for with a name that matches the source file
    get_filename_component(glslBasename "${glslSource}" NAME_WE)
    get_filename_component(glslExtension "${glslSource}" EXT)

    if(NOT DEFINED GLSL_COMPILER OR GLSL_COMPILER STREQUAL "GLSL_COMPILER-NOTFOUND")
        message(FATAL_ERROR "Cannot add Glsl shader target if GLSL_COMPILER is not set!")
    endif()

    # Add onto the target the compile command which is used to compile this glsl source file. The command changes slightly by build type. 
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_custom_command(OUTPUT "${glslBasename}${glslExtension}.spv"
            COMMAND "${GLSL_COMPILER}" "--target-env=vulkan1.0" "-x" "glsl" "-c" "-O" "${glslSource}"
            WORKING_DIRECTORY "${SHADER_BINARY_DIR}"
        )
    else()
        add_custom_command(OUTPUT "${glslBasename}${glslExtension}.spv"
            COMMAND "${GLSL_COMPILER}" "--target-env=vulkan1.0" "-x" "glsl" "-c" "-g" "-O0" "${glslSource}"
            WORKING_DIRECTORY "${SHADER_BINARY_DIR}"
        )
    endif()

    # Create the single glsl file compile target with the compiled SPIR-V as it's dependency.
    # This has the effect of linking the prior command to this target so that it will run when the target is built.
    add_custom_target(${targetName} DEPENDS "${glslBasename}${glslExtension}.spv")

endfunction(addGlslShader)