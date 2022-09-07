cmake_minimum_required(VERSION 3.22)

project(observable_memory VERSION 0.1.0 LANGUAGES CXX)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(foonathan_memory CONFIG REQUIRED)
find_package(stdsharp CONFIG REQUIRED)

include(stdsharp/Utils)

config_lib(observable_memory INTERFACE)
target_link_libraries(
    observable_memory
    INTERFACE GSL foonathan_memory stdsharp
)

target_install(
    observable_memory
    NAMESPACE "observable_memory::"
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES GSL;foonathan_memory;stdsharp
)

# add_subdirectory(tests)