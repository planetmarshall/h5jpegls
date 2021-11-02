cmake_minimum_required(VERSION 3.15)
project(h5jpegls 
    VERSION 1.0.0
    HOMEPAGE_URL https://github.com/planetmarshall/jpegls-hdf-filter
    DESCRIPTION "JPEG-LS Compression Filter"
    LANGUAGES CXX)

option(H5JPEGLS_STATIC_PLUGIN "Build the library as a static plugin to be linked into an application" OFF)
if (H5JPEGLS_STATIC_PLUGIN)
    if (CMAKE_BUILD_SHARED)
        message(STATUS "H5JPEGLS: Building Plugin as library (Shared)")
    else()
        message(STATUS "H5JPEGLS: Building Plugin as library (Static)")
    endif()
else()
    message(STATUS "H5JPEGLS: Building Plugin as dynamic module")
endif()

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
endif()

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(h5jpegls_version)
h5jpegls_configure_version_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wpedantic -Wextra -Wno-unknown-pragmas)
    add_compile_options("$<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wconversion;-Wconditional-uninitialized>")
endif()

find_package(Threads REQUIRED)
find_package(charls REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)
find_package(Catch2 QUIET)

add_subdirectory(src)
if(NOT Catch2_FOUND)
    message(WARNING "Catch2 not found - C++ tests will not be available")
else()
    enable_testing()
    add_subdirectory(test)
endif()

set(CPACK_PACKAGE_VENDOR "Andrew Marshall")
set(CPACK_PACKAGE_CONTACT "planetmarshalluk@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION "h5jpegls Compression Filter Plugin for HDF5")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
if (WIN32)
    file(TO_CMAKE_PATH "$ENV{ALLUSERSPROFILE}" ALL_USERS_PROFILE)
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/22441
    string(REPLACE "/" "\\\\" ALL_USERS_PROFILE "${ALL_USERS_PROFILE}")
    set(CPACK_NSIS_INSTALL_ROOT "${ALL_USERS_PROFILE}")
elseif(NOT CPACK_PACKAGING_INSTALL_PREFIX)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
endif()
include(CPack)
