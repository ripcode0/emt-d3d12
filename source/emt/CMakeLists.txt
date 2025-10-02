
file(GLOB_RECURSE srcs
    "{CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(emt STATIC ${srcs})
add_library(emt::d3d12 ALIAS emt)

set_target_properties(emt PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${EMT_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${EMT_BIN_DIR}"
)

target_include_directories(emt PUBLIC
    ${EMT_INC_DIR}
)

target_link_libraries(emt PUBLIC d3d12 dxgi dxguid D3DCompiler dxcompiler)

if(EMT_USE_EXPERIMENTAL)
    target_compile_definitions(emt PUBLIC USE_EXPERIMENTAL)
endif()
