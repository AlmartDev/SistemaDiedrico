include(FetchContent)

macro(LinkImGuizmo TARGET ACCESS)
    # Ensure we have the math operators defined
    target_compile_definitions(${TARGET} ${ACCESS}
        IMGUI_DEFINE_MATH_OPERATORS
    )

    FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo
        GIT_TAG master  # or specific newer commit hash
    )

    FetchContent_MakeAvailable(imguizmo)

    target_include_directories(${TARGET} ${ACCESS} 
        ${imguizmo_SOURCE_DIR}
    )

    target_sources(${TARGET} PRIVATE
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    )

    # Workaround for CaptureMouseFromApp
    target_compile_definitions(${TARGET} PRIVATE
        -DImGui::CaptureMouseFromApp=ImGui::SetNextFrameWantCaptureMouse
    )
endmacro()