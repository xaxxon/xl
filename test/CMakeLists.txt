cmake_minimum_required(VERSION 3.8)

project ("XL Test")

# Need to pick up recent versions of core language features like aligned allocations
set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)


find_package(fmt REQUIRED)
IF(FMT_FOUND)
    add_definitions(-DXL_USE_LIB_FMT)
    target_link_libraries(test-xl fmt)
ENDIF()

include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)

file(GLOB TEST_SRC
        *.cpp)

# uncomment and adjust this line to only test one part of the library
#set(TEST_SRC testing_environment.cpp test-log.cpp)

# Set address sanitizer as global flag so googletest is built with it
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -ftemplate-backtrace-limit=0")

find_package(boost)
IF(BOOST_FOUND)
    add_definitions(-DXL_USE_BOOST)
ENDIF()

IF(WIN32 AND NOT BOOST_FOUND)
    add_definition(-DXL_FORCE_NO_DEMANGLE_NAMES)
ELSE()
    IF(BOOST_FOUND)
        add_definitions(-DXL_USE_BOOST)
    ELSE()
        add_definitions(-DXL_USE_CXX_ABI)
    ENDIF()
ENDIF()

find_package(PCRE)
IF(PCRE_FOUND)
    include_directories(${PCRE_INCLUDE_DIRS})
    message("USING PCRE" ${PCRE_INCLUDE_DIRS})
    add_definitions(-DXL_USE_PCRE)
ENDIF()

include_directories(../include/xl)
add_definitions(-DXL_TESTING -DXL_LOG_WITH_TEMPLATES -DXL_TEMPLATE_LOG_ENABLE )



add_executable(test-xl EXCLUDE_FROM_ALL ${TEST_SRC})

# Turn off unused variables because clang 5 complains on structured assignments
target_compile_options(test-xl
        PRIVATE -Wall -Wextra -Werror -stdlib=libc++
        PUBLIC -msse4.1 -Wno-unused-variable
        )




add_definitions(-DGOOGLE_TEST)

add_subdirectory(googletest EXCLUDE_FROM_ALL)

target_link_libraries(test-xl c++experimental gmock gtest gmock_main xl::xl c++fs)

add_custom_target(copy_test_resources)
add_custom_command(TARGET copy_test_resources PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/templates ${PROJECT_BINARY_DIR}/templates)


add_dependencies(test-xl copy_test_resources)
target_compile_definitions(test-xl PRIVATE -DXL_TEMPLATE_LOG_ENABLE)
    