cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(example example.cpp)
find_package(h5jpegls)
target_link_libraries(example PRIVATE h5jpegls::h5jpegls)
