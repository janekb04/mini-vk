find_program(
    CMAKE_GLSL_COMPILER 
        NAMES "glslc" 
        DOC "glslc GLSL compiler" 
)
mark_as_advanced(CMAKE_GLSL_COMPILER)

set(CMAKE_GLSL_SOURCE_FILE_EXTENSIONS vert;tesc;tese;geom;frag;comp;mesh;task;glsl) # NOTE: potentially add extensions for raytracing shaders
set(CMAKE_GLSL_OUTPUT_EXTENSION .spv)
set(CMAKE_GLSL_COMPILER_ENV_VAR "GLSL")

configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeGLSLCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeGLSLCompiler.cmake)