#!/bin/bash
if [ "$1" = "static" ]; then
	echo 'CONAN_OPTIONS="-o static_plugin=True"' >> $GITHUB_ENV
elif [ "$1" = "shared" ]; then
	echo 'CONAN_OPTIONS="-o static_plugin=True" -o shared=True -o hdf5:shared=True' >> $GITHUB_ENV
fi
