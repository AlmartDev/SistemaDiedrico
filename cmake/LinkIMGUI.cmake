include(FetchContent)

macro(ImGui TARGET ACCESS)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.92.0
    )

    FetchContent_GetProperties(imgui)

    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    target_include_directories(${TARGET} ${ACCESS} 
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    # Core ImGui sources
    target_sources(${TARGET} PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    )

    if(BUILD_WEB)
        # Web-specific configuration
        target_compile_definitions(${TARGET} PRIVATE
            IMGUI_IMPL_OPENGL_ES2
            IMGUI_IMPL_OPENGL_LOADER_GLES2
            
        )
        target_sources(${TARGET} PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
    else()
        # Native configuration
        target_sources(${TARGET} PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
    endif()
endmacro()