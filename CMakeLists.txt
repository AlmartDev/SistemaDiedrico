# TODO: Finish web support!
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Web-specific configuration must come before project()
if(BUILD_WEB)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/emscripten.cmake)
    
    # Verify Emscripten is available
    if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(FATAL_ERROR "Emscripten toolchain file not found at ${CMAKE_TOOLCHAIN_FILE}")
    endif()
    
    message(STATUS "Building for WebAssembly")
endif()

project(diedrico CXX)

# Build configuration options
option(BUILD_WEB "Build for WebAssembly" OFF)
option(ENABLE_ASSIMP "Add Open Asset Import Library (assimp) to the project" ON)

# Set directory paths
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(BUILD_WEB)
    # Configure GLFW for Web
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    
    # Disable ASSIMP for web
    set(ENABLE_ASSIMP OFF)
endif()

# GLAD configuration
if(BUILD_WEB)
    set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE) 
    set(GLAD_API "gl=3.0" CACHE STRING "API version for WebGL" FORCE)  
    set(GLAD_SPEC "gl" CACHE STRING "Name of the spec" FORCE)
else()
    set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
    set(GLAD_API "gl=3.3" CACHE STRING "API version for native" FORCE)
    set(GLAD_SPEC "gl" CACHE STRING "Name of the spec" FORCE)
endif()

set(GLAD_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE STRING "Output directory" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "Language to generate the binding for" FORCE)
set(GLAD_EXTENSIONS "" CACHE STRING "Extensions to include" FORCE)
set(GLAD_ALL_EXTENSIONS OFF CACHE BOOL "Include all extensions" FORCE)
set(GLAD_NO_LOADER OFF CACHE BOOL "No loader" FORCE)
set(GLAD_REPRODUCIBLE OFF CACHE BOOL "Reproducible build" FORCE)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find all source files
file(GLOB SOURCES ${SOURCE_DIR}/*.cpp)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Apply Web-specific options
if(BUILD_WEB)
    # Emscripten-specific flags
    target_compile_options(${PROJECT_NAME} PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s USE_GLFW=3"
        "SHELL:-s USE_WEBGL2=1"
        "SHELL:-s FULL_ES3=1"
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s FORCE_FILESYSTEM=1"
        "SHELL:-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0"
        "SHELL:-s SINGLE_FILE=1"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "SHELL:-s EXPORTED_FUNCTIONS=['_main','_malloc','_free']"
        "SHELL:-O3"
    )
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-s WASM=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 --shell-file ${EMSDK}/upstream/emscripten/src/shell_minimal.html"
        SUFFIX ".html"
        OUTPUT_NAME "index"
    )
else()
    # Native build options
    find_package(OpenGL REQUIRED)
    if(OpenGL_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
    else()
        message(FATAL_ERROR "OpenGL was not found on the system")
    endif()
endif()

# Dependencies
include(${CMAKE_DIR}/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)
    
include(${CMAKE_DIR}/LinkGLAD.cmake)
LinkGLAD(${PROJECT_NAME} PRIVATE)
    
if(ENABLE_ASSIMP)
    include(${CMAKE_DIR}/LinkASSIMP.cmake)
    LinkASSIMP(${PROJECT_NAME} PRIVATE)
endif()

include(${CMAKE_DIR}/LinkIMGUI.cmake)
ImGui(${PROJECT_NAME} PRIVATE)

include(${CMAKE_DIR}/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PRIVATE)

# added json support with nlohmann/json
include(${CMAKE_DIR}/LinkJSON.cmake)
LinkJSON(${PROJECT_NAME} PRIVATE)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# C++ standard configuration
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})