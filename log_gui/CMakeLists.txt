cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /Users/xaxxon/Qt5.9.0/5.9/clang_64/lib/cmake/Qt5Widgets)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CLANG_HOME $ENV{CLANG_HOME})
link_directories(. ${CLANG_HOME}/lib)


# Find the QtWidgets library
find_package(Qt5Widgets PATHS /Users/xaxxon/Qt5.9.0/5.9/clang_64/lib/cmake/Qt5Widgets)

file(GLOB SOURCE_FILES *.cpp)

# Tell CMake to create the helloworld executable
add_executable(log_gui  ${SOURCE_FILES})

# Use the Widgets module from Qt 5.
target_link_libraries(log_gui Qt5::Widgets xl::xl fmt pcre c++experimental)

target_compile_definitions(log_gui PRIVATE -DXL_USE_PCRE)

add_custom_target(copy_example_log_status)
add_custom_command(TARGET copy_example_log_status PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/example.log_status ${CMAKE_CURRENT_BINARY_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/example2.log_status ${CMAKE_CURRENT_BINARY_DIR}/example2.log_status)


add_dependencies(log_gui copy_example_log_status)

