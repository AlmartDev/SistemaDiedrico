include(FetchContent)

macro(LinkGLFW TARGET ACCESS)
    if(BUILD_WEB)
        message(STATUS "Using Emscripten's GLFW for web build")
        
        target_compile_definitions(${TARGET} PRIVATE
            "GLFW_INCLUDE_ES3"
            "USE_WEBGL2"
            "IMGUI_IMPL_OPENGL_ES2"
        )
        
        target_link_options(${TARGET} PRIVATE
            "SHELL:-s USE_GLFW=3"
            "SHELL:-s USE_WEBGL2=1"
            "SHELL:-s FULL_ES3=1"
            # Explicitly pull in all GLFW symbols
            "SHELL:-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['\$GLFW']"
            # Enable better error reporting
            "SHELL:-s LLD_REPORT_UNDEFINED=1"
        )
        
        target_include_directories(${TARGET} ${ACCESS}
            "${EMSDK_PATH}/upstream/emscripten/cache/sysroot/include"
        )
    else()
        # Native build
        message(STATUS "Building GLFW from source for native build")
        
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build Examples" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build docs" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "Configure an install" FORCE)

        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw
            GIT_TAG 3.3.2
        )

        FetchContent_GetProperties(glfw)
        if(NOT glfw_POPULATED)
            FetchContent_Populate(glfw)
            add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR} EXCLUDE_FROM_ALL)
            set_target_properties(glfw PROPERTIES FOLDER ${PROJECT_NAME}/thirdparty)
        endif()

        target_include_directories(${TARGET} ${ACCESS} ${glfw_SOURCE_DIR}/include)
        target_link_libraries(${TARGET} ${ACCESS} glfw)
        add_dependencies(${TARGET} glfw)
    endif()
endmacro()