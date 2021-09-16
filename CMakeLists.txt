cmake_minimum_required(VERSION 3.18)
project(h5jpegls 
    VERSION 0.2.0  
    HOMEPAGE_URL https://github.com/planetmarshall/jpegls-hdf-filter
    LANGUAGES CXX)

add_library(h5jpegls MODULE h5jpegls.cc threadpool.h)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
endif()

find_package(Threads REQUIRED)
find_package(charls REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)
find_package(Catch2 QUIET)
if(NOT Catch2_FOUND)
    message(WARNING "Catch2 not found - C++ tests will not be available")
else()
    enable_testing()
    add_subdirectory(test)
endif()

target_compile_features(
   h5jpegls
   PUBLIC
   cxx_std_17
   )

target_link_libraries(
        h5jpegls
        PRIVATE
        charls::charls
        hdf5::hdf5
        Threads::Threads)

set_target_properties(h5jpegls PROPERTIES LIBRARY_OUTPUT_DIRECTORY plugins)

install(
        TARGETS h5jpegls
        EXPORT h5jpegls_targets
        LIBRARY DESTINATION hdf5/lib/plugin
)

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
endif()
include(CPack)
