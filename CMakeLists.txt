cmake_minimum_required(VERSION 3.16)
project(dnd_project VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 2.6 COMPONENTS system window graphics audio REQUIRED)

include(FetchContent)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.89.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(nlohmann_json imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)  
FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui-sfml)


file(GLOB_RECURSE DND_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM DND_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(dnd_core STATIC ${DND_SOURCES})
target_include_directories(dnd_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dnd_core PUBLIC
    sfml-graphics sfml-window sfml-audio sfml-system
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(dnd_core PUBLIC /W4 /permissive-)
else()
    target_compile_options(dnd_core PUBLIC -Wall -Wextra -Wpedantic)
endif()


add_executable(dnd_project src/main.cpp)
target_link_libraries(dnd_project PRIVATE dnd_core)


add_custom_command(TARGET dnd_project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:dnd_project>/assets)
