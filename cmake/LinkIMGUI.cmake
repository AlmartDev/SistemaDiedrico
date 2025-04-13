include(FetchContent)

macro(ImGui TARGET ACCESS)


    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.89.3
    )

    FetchContent_GetProperties(imgui)

    if (NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    target_include_directories(${TARGET} ${ACCESS} ${imgui_SOURCE_DIR})
    target_include_directories(${TARGET} ${ACCESS} ${imgui_SOURCE_DIR}/backends)

    # Add ImGui source files
    target_sources(${TARGET} PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_link_libraries(${TARGET} PRIVATE glfw OpenGL::GL)

endmacro()