cmake_minimum_required(VERSION 3.22)

project(emt-deps 
    VERSION 1.0.0 
    LANGUAGES C CXX
    DESCRIPTION "CMake Command Line External installer"
    HOMEPAGE_URL "https://github.com/ripcode0/emt-deps.git"
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(deps srcs/main.cpp)

set_target_properties(deps PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
)