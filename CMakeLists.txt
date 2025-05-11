cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(diedrico 
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "3D Geometry Application"
)

#-------------------------------------------------------------------------------
# Configuration Options
#-------------------------------------------------------------------------------

option(ENABLE_ASSIMP "Enable Open Asset Import Library (assimp) support" ON)
option(BUILD_WEB "Build for Web/Emscripten" OFF)

# Ensure EMSDK_PATH is set for web builds
if(BUILD_WEB AND NOT DEFINED EMSDK_PATH)
    set(EMSDK_PATH $ENV{EMSDK})
    if(NOT EMSDK_PATH)
        message(FATAL_ERROR "EMSDK_PATH not set for web build")
    endif()
endif()

# Enable folder organization in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------------------------
# Directory Setup
#-------------------------------------------------------------------------------

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#-------------------------------------------------------------------------------
# Source Files
#-------------------------------------------------------------------------------

file(GLOB_RECURSE SOURCES 
    ${SOURCE_DIR}/*.cpp
    ${SOURCE_DIR}/*.h
)

#-------------------------------------------------------------------------------
# Target Definition
#-------------------------------------------------------------------------------

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    FOLDER ${PROJECT_NAME}
)

if(BUILD_WEB)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wno-nontrivial-memcall"
        "-Wno-deprecated-declarations"
    )

    # Web-specific linker options
    target_link_options(${PROJECT_NAME} PRIVATE
        "SHELL:-o ${PROJECT_NAME}.html"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/shell.html"
        "SHELL:-sEXIT_RUNTIME"
        "SHELL:-s WASM=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "SHELL:-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['\$GLFW']"

        # debug
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-s SAFE_HEAP=1"
        "SHELL:-s STACK_OVERFLOW_CHECK=1"
    )
endif()

#-------------------------------------------------------------------------------
# Dependencies
#-------------------------------------------------------------------------------

# GLFW
include(${CMAKE_MODULE_PATH}/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)

if(NOT BUILD_WEB)
    include(${CMAKE_MODULE_PATH}/LinkGLAD.cmake)
    LinkGLAD(${PROJECT_NAME} PRIVATE)
endif()

# IMGUI
include(${CMAKE_MODULE_PATH}/LinkIMGUI.cmake)
ImGui(${PROJECT_NAME} PRIVATE)

# GLM
include(${CMAKE_MODULE_PATH}/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PRIVATE)

# JSON (nlohmann/json)
include(${CMAKE_MODULE_PATH}/LinkJSON.cmake)
LinkJSON(${PROJECT_NAME} PRIVATE)

# Additional include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Asset handling
if(BUILD_WEB)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
    )
endif()