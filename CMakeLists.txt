cmake_minimum_required(VERSION 3.22)

project(
  observable_memory
  VERSION 0.1.0
  LANGUAGES CXX)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(stdsharp CONFIG REQUIRED)
include(stdsharp/Utils)

config_lib(observable_memory INTERFACE)
target_link_libraries(observable_memory INTERFACE Microsoft.GSL::GSL mimalloc
                                                  stdsharp::stdsharp)

target_install(observable_memory COMPATIBILITY SameMajorVersion DEPENDENCIES
               GSL;mimalloc;stdsharp)

get_directory_property(is_subproject PARENT_DIRECTORY)

if(NOT is_subproject)
  set(is_standalone YES)
else()
  set(is_standalone NO)
endif()

include(CMakeDependentOption)

cmake_dependent_option(${PROJECT_NAME}_BUILD_TEST "Whether to build test" ON
                       "${is_standalone}" OFF)
cmake_dependent_option(
  ${PROJECT_NAME}_INCLUDE_AS_SYSTEM
  "Whether to include headers as system headers" OFF "${is_standalone}" ON)

if(${PROJECT_NAME}_INCLUDE_AS_SYSTEM)
  target_include_as_system(${PROJECT_NAME} INTERFACE)
endif()

if(${PROJECT_NAME}_BUILD_TEST)
  include(CTest)
  add_subdirectory(tests)
endif()
