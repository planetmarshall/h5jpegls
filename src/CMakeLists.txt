if (H5JPEGLS_STATIC_PLUGIN)
    add_library(h5jpegls h5jpegls.cpp h5jpegls.h)
    target_include_directories(h5jpegls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    set(H5JPEGLS_PLUGIN_DESTINATION lib)
else()
    add_library(h5jpegls MODULE h5jpegls.cpp)
    set_target_properties(h5jpegls
            PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
            LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins
            )
    set(H5JPEGLS_PLUGIN_DESTINATION hdf5/lib/plugin)
endif()

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
        LIBRARY DESTINATION ${H5JPEGLS_PLUGIN_DESTINATION}
)
