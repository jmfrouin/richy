cmake_minimum_required(VERSION 3.31)
project(richy)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(core)

add_custom_target(generate_version
        COMMAND ${CMAKE_SOURCE_DIR}/generate_version.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating version information..."
        VERBATIM
)

add_executable(richy main.cpp)

target_link_libraries(richy PRIVATE
        core
)

add_dependencies(richy generate_version)