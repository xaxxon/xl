cmake_minimum_required(VERSION 3.8)

project ("XL Test")

# Need to pick up recent versions of core language features like aligned allocations
set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)

find_package(fmt)


include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)

file(GLOB TEST_SRC
        *.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-aligned-allocation-unavailable -stdlib=libc++ -msse4.1")


include_directories(../include)
add_definitions(-DXL_USE_LIB_FMT)

add_executable(test-xl ${TEST_SRC})

set_property(TARGET test-xl PROPERTY CXX_STANDARD 17)

add_definitions(-DGOOGLE_TEST)


add_subdirectory(./googletest)

target_link_libraries(test-xl c++experimental)

add_dependencies(test-xl gmock)
add_dependencies(test-xl gtest)



target_link_libraries(test-xl gmock_main)
