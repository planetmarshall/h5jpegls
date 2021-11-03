#!/bin/bash
if [ "$1" = "shared" ]; then
	echo "CONAN_OPTIONS=-o h5jpegls:shared=True -o hdf5:shared=True" >> $GITHUB_ENV
fi
