cmake_minimum_required(VERSION 3.8)

project ("XL Test")

# Need to pick up recent versions of core language features like aligned allocations
set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)

find_package(fmt REQUIRED)

include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)

file(GLOB TEST_SRC
        *.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-aligned-allocation-unavailable -stdlib=libc++ -msse4.1 -fsanitize=address")


include_directories(../include/xl)
add_definitions(-DXL_USE_LIB_FMT)
add_definitions(-DXL_USE_PCRE) # on another line to easily add and remove during development

add_executable(test-xl EXCLUDE_FROM_ALL ${TEST_SRC} ${XL_HEADER_FILES})

add_definitions(-DGOOGLE_TEST)


add_subdirectory(googletest EXCLUDE_FROM_ALL)

target_link_libraries(test-xl c++experimental fmt gmock gtest gmock_main xl::xl)

add_custom_target(copy_test_resources)
add_custom_command(TARGET copy_test_resources PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/templates ${PROJECT_BINARY_DIR}/templates)


add_dependencies(test-xl copy_test_resources)
