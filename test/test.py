import h5py
import numpy as np

with h5py.File("test.h5", "w") as h5:
    h5.create_dataset("test", data=np.zeros((640, 512), dtype=np.uint16), compression=32012)
