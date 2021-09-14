cmake_minimum_required(VERSION 3.18)
project(h5jpegls VERSION 1.0 LANGUAGES CXX)

add_library(h5jpegls SHARED h5jpegls.cc threadpool.h)
add_library(h5jpegls::h5jpegls ALIAS h5jpegls)

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

install(
        TARGETS h5jpegls
        EXPORT h5jpegls_targets
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
