cmake_minimum_required(VERSION 3.22)

project(emt-d3d12 LANGUAGES C CXX VERSION 0.0.0)

option(EMT_USE_EXPERIMENTAL "enable the experimental features" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_INIT "-g")

if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Multi-config generator detected.")
else()
    message(STATUS "Single-config generator detected.")
endif()

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra)
endif()

set(EMT_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(EMT_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(EMT_EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(EMT_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/")
set(EMT_HLSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/hlsl/")

configure_file(
    ${EMT_INC_DIR}/emt/core/emt_path.h.in
    ${EMT_INC_DIR}/emt/core/emt_path.h
    @ONLY
)

add_subdirectory(source/emt)

add_subdirectory(source/sample)
