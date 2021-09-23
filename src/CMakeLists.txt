add_library(h5jpegls MODULE h5jpegls.cpp)
set_target_properties(h5jpegls PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
target_compile_features(h5jpegls PRIVATE cxx_std_20)
target_link_libraries(
        h5jpegls
   PRIVATE
        charls::charls
        hdf5::hdf5
        Threads::Threads
)

install(
        TARGETS h5jpegls
        EXPORT h5jpegls_targets
        LIBRARY DESTINATION hdf5/lib/plugin
)
