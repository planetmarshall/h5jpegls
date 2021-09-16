add_executable(h5jpegls_test test.cpp)
add_dependencies(h5jpegls_test h5jpegls)
target_link_libraries(h5jpegls_test
        PRIVATE
        hdf5::hdf5
	Threads::Threads
        Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(h5jpegls_test)
