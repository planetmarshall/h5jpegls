JPEG-LS HDF5 Filter
===================

The JPEG-LS HDF5 filter allows the multi-threaded compression of HDF5 datasets using the JPEG-LS codec. It is based
on original work by [Frans van den Bergh](fvdbergh@csir.co.za) and [Derick Swanepoel](dswanepoel@gmail.com)

This fork builds on that work by adding Windows support and an automated build process.

Build
-----
1. First build the CharLS library:

        git clone https://github.com/team-charls/charls.git
        cd charls
        mkdir build && cd build
        cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release ..
        make

2. Build the filter:

        cd ../..
        make

Installation
------------

Troubleshooting
---------------

During the encoding process The JPEG-LS plugin makes calls back into the HDF5 API, 
which means that applications which consume the plugin for encoding must both

* Link to the **shared** HDF5 Library
* Link to the **same** instance of the HDF5 Library

This effectively means that the plugin cannot currently be used with 
[h5py](https://docs.h5py.org/en/stable/high/dataset.html#custom-compression-filters)

See HDFGroup/hdf5#1009 and h5py/h5py#1966 for more information