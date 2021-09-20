import h5py
import numpy as np

from subprocess import check_output
from tempfile import TemporaryDirectory
import os.path


def test_file():
    file_name = os.path.realpath(os.path.join(os.path.dirname(__file__), "data", "test.h5"))
    if not os.path.exists(file_name):
        raise IOError(f"{file_name} does not exist")
    return file_name


def repack(dataset, src_file, dest_file):
    filter_id = 32012
    flag = 0
    args = ",".join(str(x) for x in [filter_id, flag])
    cmd = f"h5repack-shared --enable-error-stack -v -f {dataset}:UD={args} {src_file} {dest_file}"
    return check_output(cmd, shell=True)


def test_compress_greyscale_8():
    with h5py.File(test_file()) as h5:
        test8 = h5["test8"]
        test8_raw = np.array(test8)
        uncompressed_size = test8.id.get_storage_size()
    with TemporaryDirectory() as tempdir:
        dest_file = os.path.join(tempdir, "test.h5")
        repack("test8", test_file(), dest_file)
        with h5py.File(dest_file) as h5:
            test8 = h5["test8"]
            compressed_size = test8.id.get_storage_size()
            test8_compressed = np.array(test8)

    compression_ratio = uncompressed_size / compressed_size
    print(f"compression ratio: {compression_ratio:.3f}:1")
    assert compression_ratio > 1.5
    assert np.count_nonzero(test8_raw - test8_compressed) == 0
