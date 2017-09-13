cmake_minimum_required(VERSION 3.8)

project ("XL Test")
set(CMAKE_CXX_STANDARD 17)

set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)


include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)

file(GLOB TEST_SRC
        aligned_string.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-aligned-allocation-unavailable)


include_directories(../include)

add_executable(test-xl aligned_string.cpp)
set_property(TARGET test-xl PROPERTY CXX_STANDARD 17)

add_definitions(-DGOOGLE_TEST )

add_subdirectory(./googletest)

target_link_libraries(test-xl c++experimental)

add_dependencies(test-xl gmock)
add_dependencies(test-xl gtest)



target_link_libraries(test-xl gmock_main)
