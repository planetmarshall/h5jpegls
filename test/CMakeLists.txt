add_executable(h5jpegls_test test.cpp)
if (H5JPEGLS_STATIC_PLUGIN)
	target_link_libraries(h5jpegls_test PRIVATE h5jpegls)
	add_compile_definitions(h5jpegls_test PRIVATE H5JPEGLS_STATIC_PLUGIN)
else()
	add_dependencies(h5jpegls_test h5jpegls)
endif()

target_link_libraries(h5jpegls_test
    PRIVATE
        hdf5::hdf5
		Threads::Threads
        Catch2::Catch2)
target_compile_features(h5jpegls_test PRIVATE cxx_std_17)

include(CTest)
include(Catch)
catch_discover_tests(h5jpegls_test)
