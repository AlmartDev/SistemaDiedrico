include(FetchContent)

macro(LinkJSON TARGET ACCESS)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG v3.12.0
    )

    FetchContent_MakeAvailable(json)
    
    # Create interface library for better IDE support
    if(NOT TARGET nlohmann_json)
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE 
            $<BUILD_INTERFACE:${json_SOURCE_DIR}/single_include>
            $<INSTALL_INTERFACE:include>)
    endif()

    target_link_libraries(${TARGET} ${ACCESS} nlohmann_json)
endmacro()