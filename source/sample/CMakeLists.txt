file(GLOB_RECURSE srcs *.h *.cpp)

add_executable(sample "${srcs}")

target_link_libraries(sample PRIVATE emt::d3d12)

set_target_properties(sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EMT_BIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EMT_BIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EMT_BIN_DIR}
)