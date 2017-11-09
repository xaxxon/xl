cmake_minimum_required(VERSION 3.8)
project(xl)

set(CMAKE_VERBOSE_MAKEFILE on )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
include(clang-build-triple)


set(XL_VERSION 0.1.0)



# Contains all the header files from the library itself
file(GLOB_RECURSE XL_HEADER_FILES include/xl/*.h)


add_library(xl INTERFACE)

# so xl::xl can be used inside this project, as well
add_library(xl::xl ALIAS xl)

# Requires C++17 support
target_compile_features(xl INTERFACE cxx_std_17)
target_link_libraries(xl INTERFACE pcre)

add_subdirectory(test)
add_subdirectory(benchmark)
add_subdirectory(log_gui)


set(INCLUDE_INSTALL_DIR "include/" CACHE PATH "Installation directory for header files")

install(TARGETS xl EXPORT XL_TARGETS
        LIBRARY  DESTINATION lib
        ARCHIVE  DESTINATION lib
        RUNTIME  DESTINATION bin
        INCLUDES DESTINATION include)

install(DIRECTORY include/xl DESTINATION ${INCLUDE_INSTALL_DIR})
export(EXPORT XL_TARGETS
        FILE "${CMAKE_CURRENT_BINARY_DIR}/xlTargets.cmake"
        NAMESPACE xl::
        )

set(CONFIG_PACKAGE_LOCATION lib/cmake/xl)

# This complains about "INTERFACE_SOURCES property contains path "..." which is prefixed in the source directory"
install(EXPORT XL_TARGETS
        FILE
        xlTargets.cmake
        NAMESPACE
        xl::
        DESTINATION
        ${CONFIG_PACKAGE_LOCATION}
        )



include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/xlConfigVersion.cmake"
        VERSION $XL_VERSION
        COMPATIBILITY SameMajorVersion
)

set(CONFIG_PACKAGE_LOCATION lib/cmake/xl)

configure_package_config_file(xlConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/xlConfig.cmake
        INSTALL_DESTINATION ${CONFIG_PACKAGE_LOCATION}
        PATH_VARS INCLUDE_INSTALL_DIR XL_VERSION)


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xlConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/xlConfigVersion.cmake
        DESTINATION ${CONFIG_PACKAGE_LOCATION})
