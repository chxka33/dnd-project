cmake_minimum_required(VERSION 3.16)
project(dnd_project VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.x
    GIT_SHALLOW    TRUE)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.89.9
    GIT_SHALLOW    TRUE)

set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

# Патч imgui-sfml v2.6 для SFML 2.6.x на новых libc++ (Xcode 16+):
# toUtf8() теперь возвращает sf::U8String, а не std::basic_string<std::uint8_t>.
# Скрипт генерируется сюда же, отдельный файл не нужен. Выполняется в каталоге исходников imgui-sfml.
set(DND_IMGUI_SFML_PATCH ${CMAKE_BINARY_DIR}/patch_imgui_sfml.cmake)
file(WRITE ${DND_IMGUI_SFML_PATCH} [=[
file(READ "imgui-SFML.cpp" _content)
string(REPLACE
    "std::basic_string<std::uint8_t> tmp = sf::Clipboard::getString().toUtf8();"
    "auto tmp = sf::Clipboard::getString().toUtf8();"
    _content "${_content}")
file(WRITE "imgui-SFML.cpp" "${_content}")
]=])

FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6
    GIT_SHALLOW    TRUE
    # Совместимость с SFML 2.6.x на новых libc++ (Xcode 16+), см. скрипт выше
    PATCH_COMMAND  ${CMAKE_COMMAND} -P ${DND_IMGUI_SFML_PATCH})

FetchContent_MakeAvailable(nlohmann_json SFML imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

FetchContent_MakeAvailable(imgui-sfml)

file(GLOB_RECURSE DND_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM DND_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(dnd_core STATIC ${DND_SOURCES})
target_include_directories(dnd_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dnd_core PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(dnd_core PUBLIC /W4 /permissive-)
else()
    target_compile_options(dnd_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

add_executable(dnd_project src/main.cpp)
target_link_libraries(dnd_project PRIVATE
    dnd_core
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML)

add_custom_command(TARGET dnd_project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:dnd_project>/assets)

# Юнит-тесты (Catch2): cmake -DDND_BUILD_TESTS=ON ..
option(DND_BUILD_TESTS "Build unit tests" OFF)
if(DND_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    file(GLOB DND_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
    add_executable(dnd_tests ${DND_TEST_SOURCES})
    target_link_libraries(dnd_tests PRIVATE dnd_core Catch2::Catch2WithMain)

    include(CTest)
    include(Catch)
    catch_discover_tests(dnd_tests)
endif()