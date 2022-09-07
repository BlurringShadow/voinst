#
# Project details
#
project(${CMAKE_PROJECT_NAME}Tests LANGUAGES CXX)

verbose_message("Adding tests under ${CMAKE_PROJECT_NAME}...")

#
# Set the sources for the unit tests and add the executable(s)
#
set(
    src
    src/tests.cpp
)

config_lib(${PROJECT_NAME}Lib INTERFACE)
config_exe(${PROJECT_NAME} EXE_SRC ${src})

target_compile_options(
    ${PROJECT_NAME}
    PUBLIC $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Werror>
           $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

find_package(Catch2 CONFIG REQUIRED)
include(Catch)
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain ${PROJECT_NAME}Lib)

#
# Configure clang-tidy
#
option(ENABLE_CLANG_TIDY "Enable clang-tidy" ON)

if(${ENABLE_CLANG_TIDY})
    message(STATUS "clang-tidy is enabled")

    target_enable_clang_tidy(${PROJECT_NAME})
endif()

catch_discover_tests(${PROJECT_NAME})

#
# Configure code coverage
#
option(ENABLE_CODE_COVERAGE "Enable code coverage" OFF)

if(${ENABLE_CODE_COVERAGE})
    target_coverage(${PROJECT_NAME} FORMAT lcov)
endif()