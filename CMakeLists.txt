cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(diedrico 
    LANGUAGES CXX
    VERSION 1.0.0
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

# Handle assets differently for web vs native builds
if(BUILD_WEB)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sASYNCIFY ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sASYNCIFY -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    
    # Embed assets directory for Emscripten
    get_filename_component(ABSOLUTE_ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets" ABSOLUTE)
    target_link_options(${PROJECT_NAME} PRIVATE
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/shell.html"
        "SHELL:-sWASM=1"
        "SHELL:-sMIN_WEBGL_VERSION=2"
        "SHELL:-sMAX_WEBGL_VERSION=2"
        "SHELL:-sASSERTIONS=1"
        "SHELL:-sGL_DEBUG=1"
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sDISABLE_EXCEPTION_CATCHING=0"
        "SHELL:--bind"
        "SHELL:--preload-file ${ABSOLUTE_ASSETS_PATH}@/assets"
    )

else()
    # Copy assets folder to executable directory for native builds
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
        COMMENT "Copying assets folder to executable directory"
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

# ImGuiFileDialog (only for non-Windows and non-Web builds)
include(${CMAKE_MODULE_PATH}/LinkImGuiFileDialog.cmake)
LinkImGuiFileDialog(${PROJECT_NAME} PRIVATE)

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