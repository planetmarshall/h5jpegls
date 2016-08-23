JPEG-LS HDF5 Filter
===================
The JPEG-LS HDF5 filter allows the multi-threaded compression of HDF5 datasets using the JPEG-LS codec.

Dependencies
------------
The filter depends on the CharLS implementation of JPEG-LS which can be obtained from https://github.com/team-charls/charls

Building
--------
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
Copy or symlink the filter to `/usr/local/hdf5/lib/plugin`. Note that since the filter is linked to libCharLS using its absolute path at compile time, libCharLS needs to be available at that location.
