cmake_minimum_required(VERSION 3.21)

project(mini-vk)

function(target_shaders target_name paths_in)
    set(paths_out "")
    foreach(path_in_raw ${paths_in})
        set(path_in "${CMAKE_CURRENT_SOURCE_DIR}/${path_in_raw}")
        set(path_out "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${path_in_raw}.spv")
        add_custom_command(OUTPUT ${path_out}
            COMMAND glslc ${path_in} -o ${path_out}
            DEPENDS ${path_in}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Compiling GLSL shader" ${path_in})
        list(APPEND paths_out ${path_out})
    endforeach()
    add_custom_target("${target_name}_shaders" DEPENDS ${paths_out})
    add_dependencies(${target_name} "${target_name}_shaders")
endfunction()

add_subdirectory(external/glfwpp)
add_subdirectory(external/mimalloc)
find_package(Vulkan REQUIRED FATAL_ERROR)

add_executable(mini-vk main.cpp)
target_shaders(mini-vk "basic.vert;basic.frag")
target_link_libraries(mini-vk PRIVATE GLFWPP Vulkan::Headers mimalloc-static)
target_compile_features(mini-vk PRIVATE cxx_std_20)

# Compiler specific options
if(MSVC)
    target_compile_options(mini-vk PRIVATE "/W4") # NOTE: add "/WX" to treat warnings as errors
    #target_link_options(mini-vk PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup") # launch with no terminal window
else()
    target_compile_options(mini-vk PRIVATE "-Wall" "-Wextra") # NOTE: add "-Werror" to treat warnings as errors
endif()