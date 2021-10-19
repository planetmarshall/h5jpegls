JPEG-LS HDF5 Filter
===================

![CI Build](https://github.com/planetmarshall/jpegls-hdf-filter/actions/workflows/cmake-build.yml/badge.svg)

The JPEG-LS HDF5 filter allows the compression of HDF5 datasets using the 
[JPEG-LS](https://jpeg.org/jpegls/) codec. It is based
on original work by _Frans van den Bergh_ and _Derick Swanepoel_. The 
[CharLS](https://github.com/team-charls/charls) JPEG-LS implementation is used internally.

This implementation remains backward-compatible with the original version and so retains 
the [registered filter](https://portal.hdfgroup.org/display/support/Filters) ID `32012`.

Limitations
-----------

* Only integer typed datasets of up to 16 bits in size are supported. This reflects the 
  features of the JPEG-LS Codec. For other datasets other compression algorithms, such 
  as [ZFP](https://github.com/LLNL/H5Z-ZFP) may be more suitable.
* Only datasets chunked in 2 dimensions are supported (ie, greyscale images). Multi-component
  images will be supported in a future release.

Build
-----
1. Install [conan](https://docs.conan.io/en/latest/)
   ```
   pip install conan
   ```
2. Install dependencies
   ```
   mkdir build
   conan install . -if build --build missing
   ```
3. Build with cmake
   ```
   cd build
   cmake .. -D CMAKE_BUILD_TYPE=Release
   cmake --build .
   ```
4. Run tests
   ```
   cd build
   export HDF5_PLUGIN_PATH=$(pwd)/plugins
   ctest --extra-verbose
   ```
   
Build as a Library
------------------

Normally HDF5 Plugins are built as dynamic libraries and discovered by an application at runtime. However
they can also be built as a library and linked to an application normally. This has the advantage of
working with both static and shared HDF5 library variants.

To build as a library, configure cmake with `-DH5JPEGLS_STATIC_PLUGIN=ON`

To use the filter as a library it must be manually registered by calling `h5jpegls_register_plugin()`. This
step is not necessary if using the filter as a dynamic plugin.

Installation
------------

1. Build as above and generate a package
   ```
   cd build
   cpack -G DEB .
   ```
   Or on Windows
   ```
   cpack -G NSIS .
   ```
2. Install the package according to your OS. The generated packages install the plugin in the
   default search path. HDF5 finds dynamic plugins in non-default locations by 
   searching the path in the environment variable `HDF5_PLUGIN_PATH`.

Backwards Compatibility
-----------------------

The codec retains the ability to decode datasets created with the original codec, however
this implementation performs compression in a different way - it no longer divides chunks
into "stripes" for parallel compression as this behaviour comes at the 
cost of a poorer compression ratio. Instead, chunks are compressed in their entirety.

Troubleshooting
---------------

During the encoding process The JPEG-LS plugin makes calls back into the HDF5 API, 
which means that applications which consume the plugin for encoding must both:

* Link to the **shared** HDF5 Library
* Link to the **same** instance of the HDF5 Library

This effectively means that the plugin cannot currently be used with 
[h5py](https://docs.h5py.org/en/stable/high/dataset.html#custom-compression-filters)

See [HDF5 Issue 1009](https://github.com/HDFGroup/hdf5/issues/1009) and 
[h5py issue 1966](https://github.com/h5py/h5py/issues/1966) for more information