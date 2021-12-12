set(H5JPEGLS_SOURCES h5jpegls.cpp version.hpp ${CMAKE_SOURCE_DIR}/include/h5jpegls/h5jpegls.h)
if (H5JPEGLS_BUILD_LIBRARY)
    add_library(h5jpegls h5jpegls.cpp ${H5JPEGLS_SOURCES})
    set_target_properties(h5jpegls PROPERTIES PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/h5jpegls/h5jpegls.h)
    target_include_directories(h5jpegls PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
            )
    set(H5JPEGLS_PLUGIN_DESTINATION lib)
    set_target_properties(h5jpegls PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    add_library(h5jpegls MODULE ${H5JPEGLS_SOURCES})
    target_include_directories(h5jpegls PRIVATE ${CMAKE_SOURCE_DIR}/include)
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
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        PUBLIC_HEADER DESTINATION include/h5jpegls
)
