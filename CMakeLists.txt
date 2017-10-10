cmake_minimum_required(VERSION 3.8)
project(xl)

set(XL_VERSION 0.1.0)


set(CMAKE_VERBOSE_MAKEFILE on )


SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)
include(clang-build-triple)


set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

file(GLOB XL_HEADER_FILES include/*.h include/aligned_string/*.h include/template/*.h )

message("${XL_HEADER_FILES}")

add_library(xl INTERFACE)
#target_sources(xl INTERFACE ${XL_HEADER_FILES})


add_subdirectory(test)
add_subdirectory(benchmark)

add_definitions(-DXL_USE_LIB_FMT)


set(INCLUDE_INSTALL_DIR "include/xl" CACHE PATH "Installation directory for header files")

install(TARGETS xl EXPORT XL_TARGETS
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(FILES ${XL_HEADER_FILES} DESTINATION ${INCLUDE_INSTALL_DIR})
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
