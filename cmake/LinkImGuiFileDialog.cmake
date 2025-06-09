include(FetchContent)

macro(LinkImGuiFileDialog TARGET ACCESS)
    if(NOT BUILD_WEB) # NOT WIN32 AND 
        message(STATUS "Including ImGuiFileDialog (non-Windows, non-Web build)")
        
        FetchContent_Declare(
            imgui_filedialog
            GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
            GIT_TAG v0.6.7  
            SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/imgui_filedialog-src"
        )

        FetchContent_GetProperties(imgui_filedialog)
        if(NOT imgui_filedialog_POPULATED)
            FetchContent_Populate(imgui_filedialog)
        endif()

        # Add the single implementation file
        set(IMGUI_FD_SOURCE "${imgui_filedialog_SOURCE_DIR}/ImGuiFileDialog.cpp")
        
        if(NOT EXISTS "${IMGUI_FD_SOURCE}")
            message(FATAL_ERROR "ImGuiFileDialog.cpp not found at: ${IMGUI_FD_SOURCE}")
        endif()

        # Add include directory and source file
        target_include_directories(${TARGET} ${ACCESS} 
            ${imgui_filedialog_SOURCE_DIR}
        )

        target_sources(${TARGET} PRIVATE
            ${IMGUI_FD_SOURCE}
        )

        message(STATUS "ImGuiFileDialog added from: ${imgui_filedialog_SOURCE_DIR}")
    else()
        message(STATUS "Skipping ImGuiFileDialog (Windows or Web build)")
    endif()
endmacro()