cmake_minimum_required(VERSION 3.21)
project(mini-vk)

add_subdirectory(external/glfwpp)
add_subdirectory(external/mimalloc)
find_package(Vulkan REQUIRED FATAL_ERROR)

add_executable(mini-vk main.cpp)
target_link_libraries(mini-vk PRIVATE GLFWPP Vulkan::Headers mimalloc-static)
target_compile_features(mini-vk PRIVATE cxx_std_20)

# Compiler specific options
if(MSVC)
    target_compile_options(mini-vk PRIVATE "/W4") # NOTE: add "/WX" to treat warnings as errors
    target_link_options(mini-vk PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup") # launch with no terminal window
else()
    target_compile_options(mini-vk PRIVATE "-Wall" "-Wextra") # NOTE: add "-Werror" to treat warnings as errors
endif()