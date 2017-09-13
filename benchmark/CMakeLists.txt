cmake_minimum_required(VERSION 3.8)

project ("XL Test")
set(CMAKE_CXX_STANDARD 17)

set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)


include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)

file(GLOB TEST_SRC
        aligned_string.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-aligned-allocation-unavailable -stdlib=libc++ -msse4.1 -O3")


include_directories(../include)

add_executable(bench-xl aligned_string.cpp)
set_property(TARGET bench-xl PROPERTY CXX_STANDARD 17)



target_link_libraries(bench-xl c++experimental benchmark)
