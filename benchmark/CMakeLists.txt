cmake_minimum_required(VERSION 3.8)

project ("XL Bench")

set(CLANG_HOME $ENV{CLANG_HOME})

# need to pick up c++17-compatible core c++ libraries
link_directories(. ${CLANG_HOME}/lib)

file(GLOB BENCH_SRC
        *.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-aligned-allocation-unavailable -stdlib=libc++ -msse4.1 -O3")

# EXCLUDE_FROM_ALL so it doesn't get built on make install
add_executable(bench-xl EXCLUDE_FROM_ALL ${BENCH_SRC})
target_include_directories(bench-xl PRIVATE ../include/xl)

target_link_libraries(bench-xl c++experimental benchmark fmt xl::xl)
