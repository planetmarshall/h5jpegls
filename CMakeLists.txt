cmake_minimum_required(VERSION 3.19)
project(h5jpegls VERSION 1.0 LANGUAGES CXX)

add_library(h5jpegls SHARED h5jpegls.cc threadpool.h)

if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
endif()

find_package(charls REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)

target_link_libraries(h5jpegls PRIVATE charls::charls hdf5::hdf5)

install(
        TARGETS h5jpegls
        EXPORT h5jpegls_targets
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
