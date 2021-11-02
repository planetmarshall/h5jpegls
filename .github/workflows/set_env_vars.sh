#!/bin/bash
if [ "$1" = "static" ]; then
	echo "CONAN_OPTIONS=-o enable_tests=True" >> $GITHUB_ENV
elif [ "$1" = "shared" ]; then
	echo "CONAN_OPTIONS=-o shared=True -o hdf5:shared=True -o enable_tests=True" >> $GITHUB_ENV
elif [ "$1" = "module" ]; then
	echo "CONAN_OPTIONS=-o static_plugin=False -o enable_tests=True" >> $GITHUB_ENV
fi
