cmake_minimum_required(VERSION 3.20)
project(chip8 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL2_DIR "${CMAKE_SOURCE_DIR}/external/SDL2-2.32.10/cmake")
find_package(SDL2 CONFIG REQUIRED)

add_executable(chip8 src/main.cpp src/chip8.cpp)
target_include_directories(chip8 PRIVATE include)
target_link_libraries(chip8 PRIVATE SDL2::SDL2)

add_custom_command(TARGET chip8 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/external/SDL2-2.32.10/lib/x64/SDL2.dll"
    $<TARGET_FILE_DIR:chip8>)
