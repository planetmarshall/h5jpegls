#include <H5PLextern.h>
#include <H5Zpublic.h>
#include <hdf5.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <vector>

#include <charls/charls.h>

#include "threadpool.h"
ThreadPool* filter_pool = nullptr;

#include <future>

using std::vector;

// Temporary unofficial filter ID
const H5Z_filter_t H5Z_FILTER_JPEGLS = 32012;

int get_threads() {
    int threads = 0;
    char* envvar = getenv("HDF5_FILTER_THREADS");
    if (envvar != NULL) {
        threads = atoi(envvar);
    }
    if (threads <= 0) {
        threads = std::min(std::thread::hardware_concurrency(), 8u);
    }
    return threads;
}

size_t codec_filter(unsigned int flags, size_t cd_nelmts,
    const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf) {

    int threads = get_threads();
    if (filter_pool == nullptr || filter_pool->get_threads() != threads) {
        delete filter_pool;
        filter_pool = new ThreadPool(threads);
    }
    filter_pool->lock_buffers();

    int length = 1;
    size_t nblocks = 32; // number of time series if encoding time-major chunks
    int typesize = 1;
    if (cd_nelmts > 2 && cd_values[0] > 0) {
        length   = cd_values[0];
        nblocks  = cd_values[1];
        typesize = cd_values[2];
    } else {
        printf("Error: Incorrect number of filter parameters specified. Aborting.\n");
        return -1;
    }
    char errMsg[256];

    size_t subchunks = std::min(size_t(24), nblocks);
    const size_t lblocks = nblocks / subchunks;
    const size_t header_size = 4*subchunks;
    const size_t remainder = nblocks - lblocks * subchunks;

    if (flags & H5Z_FLAG_REVERSE) {
        /* Input */
        unsigned char* in_buf = (unsigned char*)realloc(*buf, nblocks*length*typesize*2);
        *buf = in_buf;

        std::vector<uint32_t> block_size(subchunks);
        std::vector<uint32_t> offset(subchunks);
        // Extract header
        memcpy(block_size.data(), in_buf, subchunks*sizeof(uint32_t));

        offset[0] = 0;
        uint32_t coffset = 0;
        for (size_t block=1; block < subchunks; block++) {
            coffset += block_size[block-1];
            offset[block] = coffset;
        }

        std::vector<unsigned char*> tbuf(subchunks);
        vector< std::future<void> > futures;
        // Make a copy of the compressed buffer. Required because we
        // now realloc in_buf.
        for (size_t block=0; block < subchunks; block++) {
            futures.emplace_back(
                filter_pool->enqueue( [&,block] {
                    tbuf[block] = filter_pool->get_global_buffer(block, length*nblocks*typesize + 512);
                    memcpy(tbuf[block], in_buf+header_size+offset[block], block_size[block]);
                })
            );
        }
        // must wait for copies to complete, otherwise having
        // threads > subchunks could lead to a decompressor overwriting in_buf
        for (size_t i=0; i < futures.size(); i++) {
            futures[i].wait();
        }

        for (size_t block=0; block < subchunks; block++) {
            futures.emplace_back(
                filter_pool->enqueue( [&,block] {
                    size_t own_blocks = (block < remainder ? 1 : 0) + lblocks;
                    CharlsApiResultType ret = JpegLsDecode(
                        in_buf + typesize*length* ( (block < remainder) ? block*(lblocks+1) : (remainder*(lblocks+1) + (block-remainder)*lblocks) ),
                        typesize*length*own_blocks,
                        tbuf[block],
                        block_size[block],
                        nullptr,
                        errMsg
                    );
                    if (ret != CharlsApiResultType::OK) {
                        fprintf(stderr, "JPEG-LS error %d: %s\n", ret, errMsg);
                    }
                })
            );
        }
        for (size_t i=0; i < futures.size(); i++) {
            futures[i].wait();
        }

        *buf_size = nblocks*length*typesize;

        filter_pool->unlock_buffers();
        return *buf_size;

    } else {
        /* Output */

        unsigned char* in_buf = (unsigned char*)*buf;

        std::vector<uint32_t> block_size(subchunks);
        std::vector<unsigned char*> local_out(subchunks);

        vector< std::future<void> > futures;
        for (size_t block=0; block < subchunks; block++) {
            size_t own_blocks = (block < remainder ? 1 : 0) + lblocks;
            local_out[block] = filter_pool->get_global_buffer(block, own_blocks*length*typesize + 8192);

            futures.emplace_back(
                filter_pool->enqueue( [&,block,own_blocks] {
                    JlsParameters params = JlsParameters();
                    params.width = length;
                    params.height = own_blocks;
                    params.bitsPerSample = typesize * 8;
                    params.components = 1;
                    size_t csize;
                    CharlsApiResultType ret = JpegLsEncode(
                        local_out[block],
                        own_blocks*length*typesize + 8192,
                        &csize,
                        in_buf + typesize*length* ( (block < remainder) ? block*(lblocks+1) : (remainder*(lblocks+1) + (block-remainder)*lblocks) ),
                        own_blocks*length*typesize,
                        &params,
                        errMsg
                    );
                    if (ret != CharlsApiResultType::OK) {
                        fprintf(stderr, "JPEG-LS error: %s\n", errMsg);
                    }
                    block_size[block] = csize;
                })
            );
        }
        for (size_t i=0; i < futures.size(); i++) {
            futures[i].wait();
        }

        size_t compr_size = header_size;
        for (size_t block=0; block < subchunks; block++) {
            compr_size += block_size[block];
        }

        if (compr_size > nbytes) {
            in_buf = (unsigned char*)realloc(*buf, compr_size);
            *buf = in_buf;
        }

        memcpy(in_buf, (unsigned char*)block_size.data(), header_size);
        size_t offset = header_size;
        for (size_t block=0; block < subchunks; block++) {
            memcpy(in_buf + offset, local_out[block], block_size[block]);
            offset += block_size[block];
        }

        size_t compressed_size = offset;
        *buf_size = compressed_size;

        filter_pool->unlock_buffers();
        return compressed_size;

    }
}

herr_t h5jpegls_set_local(hid_t dcpl_id, hid_t type_id, hid_t chunk_space_id) {
    unsigned int flags;
    size_t nelements = 8;
    unsigned int values[] = {0,0,0,0,0,0,0,0};
    herr_t r = H5Pget_filter_by_id(dcpl_id, H5Z_FILTER_JPEGLS, &flags, &nelements, values, 0, NULL, NULL);

    if (r < 0) {
        return -1;
    }

    // TODO: if some parameters were passed (e.g., number of subchunks)
    // we should extract them here

    hsize_t chunkdims[32];
    int ndims = H5Pget_chunk(dcpl_id, 32, chunkdims);
    if (ndims < 0) {
        return -1;
    }

    int byte_mode = nelements > 0 && values[0] != 0;

    values[0] = chunkdims[ndims-1];
    values[1] = (ndims == 1) ? 1 : chunkdims[ndims-2];

    unsigned int typesize = H5Tget_size(type_id);
    if (typesize == 0) {
        return -1;
    }

    H5T_class_t classt = H5Tget_class(type_id);
    if (classt == H5T_ARRAY) {
        hid_t super_type = H5Tget_super(type_id);
        typesize = H5Tget_size(super_type);
        H5Tclose(super_type);
    }

    if (byte_mode) {
        values[2] = 1;
        values[0] *= typesize;
    } else {
        values[2] = typesize;
    }

    nelements = 3; // TODO: update if we accept #subchunks
    r = H5Pmodify_filter(dcpl_id, H5Z_FILTER_JPEGLS, flags, nelements, values);

    if (r < 0) {
        return -1;
    }

    return 1;
}

extern "C" const H5Z_class2_t H5Z_JPEGLS[1] = {{
    H5Z_CLASS_T_VERS,                 /* H5Z_class_t version */
    (H5Z_filter_t)H5Z_FILTER_JPEGLS,         /* Filter id number */
    1,              /* encoder_present flag (set to true) */
    1,              /* decoder_present flag (set to true) */
    "HDF5 JPEG-LS filter v0.2.1 <https://github.com/planetmarshall/jpegls-hdf-filter>", /* Filter name for debugging */
    NULL,           /* The "can apply" callback     */
    (H5Z_set_local_func_t)(h5jpegls_set_local),           /* The "set local" callback */
    (H5Z_func_t)codec_filter,         /* The actual filter function */
}};

extern "C" H5PL_type_t H5PLget_plugin_type(void) {
    return H5PL_TYPE_FILTER; 
}
extern "C" const void *H5PLget_plugin_info(void) {
    return H5Z_JPEGLS;
}

#ifndef _MSC_VER
__attribute__((destructor)) void destroy_threadpool() {
    delete filter_pool;
}
#endif

