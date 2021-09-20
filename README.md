JPEG-LS HDF5 Filter
===================

The JPEG-LS HDF5 filter allows the multi-threaded compression of HDF5 datasets using the JPEG-LS codec. It is based
on original work by [Frans van den Bergh](fvdbergh@csir.co.za) and [Derick Swanepoel](dswanepoel@gmail.com)

This fork builds on that work by adding Windows support and an automated build process. This version has
been created to serve as a baseline for future development

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