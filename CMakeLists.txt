cmake_minimum_required(VERSION 3.22)
#
# Project details
#
project(YOUR_PROJECT_NAME VERSION 0.0.1 LANGUAGES CXX)

#
# Model project dependencies
#
find_package(stdsharp CONFIG REQUIRED)

include(stdsharp/Utils)

config_lib(${PROJECT_NAME} INTERFACE STD 23)

target_compile_options(
    ${PROJECT_NAME}
    INTERFACE $<$<CXX_COMPILER_ID:MSVC>: /external:anglebrackets>
)

target_link_libraries(
    ${PROJECT_NAME}
    INTERFACE stdsharp::stdsharp
)

verbose_message("Successfully added all dependencies and linked against them.")

target_install(
    ${PROJECT_NAME}
    NAMESPACE ${PROJECT_NAME}
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES stdsharp
)

get_directory_property(is_subproject PARENT_DIRECTORY)

if(NOT is_subproject)
    set(is_standalone YES)
else()
    set(is_standalone NO)
endif()

include(CMakeDependentOption)

CMAKE_DEPENDENT_OPTION(${PROJECT_NAME}_BUILD_TEST "Whether to build test" ON "${is_standalone}" OFF)
CMAKE_DEPENDENT_OPTION(${PROJECT_NAME}_INCLUDE_AS_SYSTEM "Whether to include headers as system headers" OFF "${is_standalone}" ON)

if(${PROJECT_NAME}_INCLUDE_AS_SYSTEM)
    target_include_as_system(${PROJECT_NAME} INTERFACE)
endif()

if(${PROJECT_NAME}_BUILD_TEST)
    include(CTest)
    add_subdirectory(tests)
endif()