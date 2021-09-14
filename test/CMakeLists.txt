add_executable(h5jpegls_test test.cpp)
target_link_libraries(h5jpegls_test
        PRIVATE
        hdf5::hdf5
        h5jpegls::h5jpegls
        Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(h5jpegls_test)