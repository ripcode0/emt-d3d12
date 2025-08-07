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

target_compile_definitions(deps PRIVATE
    EMT_DEPS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    EMT_DEPS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    EMT_DEPS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set_target_properties(deps PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
)