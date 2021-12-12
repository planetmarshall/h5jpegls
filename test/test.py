from subprocess import check_output
from tempfile import TemporaryDirectory
import os.path
import logging

import h5py
import numpy as np
import pytest


def data_file(file_name):
    file_path = os.path.realpath(os.path.join(os.path.dirname(__file__), "data", file_name))
    if not os.path.exists(file_path):
        raise IOError(f"{file_path} does not exist")
    return file_path


def repack(dataset, src_file, dest_file):
    filter_id = 32012
    flag = 1
    args = ",".join(str(x) for x in [filter_id, flag])
    cmd = f"h5repack-shared --enable-error-stack -v -f {dataset}:UD={args} {src_file} {dest_file}"
    return check_output(cmd.split())


def unpack(dataset, src_file, dest_file):
    cmd = f"h5repack-shared --enable-error-stack -v -f {dataset}:NONE {src_file} {dest_file}"
    return check_output(cmd.split())


@pytest.mark.parametrize("dataset", ["test8", "test16"])
def test_compress_greyscale(dataset):
    with h5py.File(data_file("test.h5")) as h5:
        test_dataset = h5[dataset]
        test_raw = np.array(test_dataset)
        uncompressed_size = test_dataset.id.get_storage_size()
    with TemporaryDirectory() as tempdir:
        dest_file = os.path.join(tempdir, "test_compressed.h5")
        repack(dataset, data_file("test.h5"), dest_file)
        with h5py.File(dest_file, "a") as h5:
            test8 = h5[dataset]
            compressed_size = test8.id.get_storage_size()
            test8gz = h5.create_dataset(dataset + "gz", data=test8, compression="gzip")
            gzip_compressed_size = test8gz.id.get_storage_size()
            test_compressed = np.array(test8)

    compression_ratio = uncompressed_size / compressed_size
    gzip_compression_ratio = uncompressed_size / gzip_compressed_size
    logging.info(f"compression ratio: {compression_ratio:.3f}:1")
    logging.info(f"GZIP compression ratio: {gzip_compression_ratio:.3f}:1")
    assert compressed_size < gzip_compressed_size
    assert np.count_nonzero(test_raw - test_compressed) == 0


def test_compress_rgb():
    dataset = "test8_rgb"
    with h5py.File(data_file("test.h5")) as h5:
        test_dataset = h5[dataset]
        test_raw = np.array(test_dataset)
        uncompressed_size = test_dataset.id.get_storage_size()
    with TemporaryDirectory() as tempdir:
        dest_file = os.path.join(tempdir, "test_compressed.h5")
        repack(dataset, data_file("test.h5"), dest_file)
        with h5py.File(dest_file, "a") as h5:
            test8 = h5[dataset]
            test8gz = h5.create_dataset(dataset + "gz", data=test8, compression="gzip")
            gzip_compressed_size = test8gz.id.get_storage_size()
            compressed_size = test8.id.get_storage_size()
            test_compressed = np.array(test8)

    compression_ratio = uncompressed_size / compressed_size
    gzip_compression_ratio = uncompressed_size / gzip_compressed_size
    logging.info(f"compression ratio: {compression_ratio:.3f}:1")
    logging.info(f"GZIP compression ratio: {gzip_compression_ratio:.3f}:1")
    assert compressed_size < gzip_compressed_size
    assert np.count_nonzero(test_raw - test_compressed) == 0


@pytest.mark.parametrize("dataset_name", ["test8", "test16"])
def test_backwards_compatibility(dataset_name):
    with h5py.File(data_file("test_compressed_v0.2.h5")) as h5:
        dataset = h5[dataset_name]
        data = np.array(dataset)
        compressed_size = dataset.id.get_storage_size()

    with h5py.File(data_file("test.h5")) as h5:
        dataset = h5[dataset_name]
        expected_data = np.array(dataset)
        original_size = dataset.id.get_storage_size()

    assert compressed_size < original_size
    assert np.count_nonzero(expected_data - data) == 0


def _file_size(path):
    stat = os.stat(os.path.realpath(path))
    return stat.st_size


def test_h5repack_roundtrip():
    dataset = "test8"
    with h5py.File(data_file("test.h5")) as h5:
        expected_data = np.array(h5[dataset])

    logging.info(f"Original file size: {_file_size(data_file('test.h5'))} bytes")

    with TemporaryDirectory() as tempdir:
        packed_file = os.path.join(tempdir, "test_packed.h5")
        repack(dataset, data_file("test.h5"), packed_file)
        packed_file_size = _file_size(packed_file)
        logging.info(f"Packed file size: {packed_file_size} bytes")

        unpacked_file = os.path.join(tempdir, "test_unpacked.h5")
        unpack(dataset, packed_file, unpacked_file)
        unpacked_file_size = _file_size(unpacked_file)
        logging.info(f"Unpacked file size: {unpacked_file_size} bytes")

        assert(unpacked_file_size > packed_file_size)

        with h5py.File(unpacked_file) as h5:
            actual_data = np.array(h5[dataset])

    assert np.count_nonzero(expected_data - actual_data) == 0
