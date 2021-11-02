#!/bin/bash
if [ "$1" = "shared" ]; then
	echo "CONAN_OPTIONS=-o shared=True -o hdf5:shared=True" >> $GITHUB_ENV
fi
