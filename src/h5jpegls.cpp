#include <H5PLextern.h>
#include <H5Zpublic.h>
#include <hdf5.h>
#include <charls/charls.h>

#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>

// Temporary unofficial filter ID
const H5Z_filter_t H5Z_FILTER_JPEGLS = 32012;

size_t decode(void **buffer, size_t *buffer_size, size_t data_size) {
    charls::jpegls_decoder decoder;
    decoder.source(*buffer, data_size);
    decoder.read_header();
    auto required_bytes = decoder.destination_size();
    std::vector<uint8_t> decoding_buffer(required_bytes);
    decoder.decode(decoding_buffer);
    if (*buffer_size < required_bytes) {
        *buffer = realloc(*buffer, required_bytes);
        *buffer_size = required_bytes;
    }
    memcpy(*buffer, decoding_buffer.data(), required_bytes);
    return required_bytes;
}

size_t encode(void **buffer, size_t *buffer_size, size_t data_size, int bytes_per_element, uint32_t chunk_width, uint32_t chunk_height) {
    charls::jpegls_encoder encoder;
    charls::frame_info frame{
        .width = chunk_width,
        .height = chunk_height,
        .bits_per_sample = bytes_per_element * 8,
        .component_count = 1
    };
    encoder.frame_info(frame);
    std::vector<uint8_t> encoding_buffer(*buffer_size);
    encoder.destination(encoding_buffer);
    auto num_encoded_bytes = encoder.encode(*buffer, data_size);
    assert(num_encoded_bytes < *buffer_size);
    *buffer_size = num_encoded_bytes;
    memcpy(*buffer, encoding_buffer.data(), num_encoded_bytes);
    return num_encoded_bytes;
}

htri_t can_apply_filter(hid_t dcpl_id, hid_t type_id, hid_t space_id) {
    constexpr hsize_t max_rank = 32;
    std::array<hsize_t, max_rank> chunk_dimensions{};
    auto rank = H5Pget_chunk(dcpl_id, 32, chunk_dimensions.data());
    if (rank != 2) {
        return 0;
    }
    auto type_class = H5Tget_class(type_id);
    bool can_filter = type_class == H5T_INTEGER;
    if (type_class == H5T_ARRAY) {
        auto base_class = H5Tget_super(type_id);
        can_filter = base_class == H5T_INTEGER;
    }
    return can_filter ? 1 : 0;
}

size_t codec_filter(unsigned int flags, size_t cd_nelmts,
                    const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf) {

    int width = cd_values[0];
    int height = cd_values[1];
    int typesize = cd_values[2];

    try {
        if (flags & H5Z_FLAG_REVERSE) {
            return decode(buf, buf_size, nbytes);
        }
        return encode(buf, buf_size, nbytes, typesize, width, height);
    } catch (const std::exception & ex) {
        fprintf(stderr, ex.what());
        fprintf(stderr, "\n");
    } catch (...) {
        fprintf(stderr, "Unknown error\n");
    }
    return 0;
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
    "HDF5 JPEG-LS filter v1.0.0 <https://github.com/planetmarshall/jpegls-hdf-filter>", /* Filter name for debugging */
    can_apply_filter,           /* The "can apply" callback     */
    h5jpegls_set_local,           /* The "set local" callback */
    codec_filter,         /* The actual filter function */
}};

extern "C" H5PL_type_t H5PLget_plugin_type(void) {
    return H5PL_TYPE_FILTER; 
}
extern "C" const void *H5PLget_plugin_info(void) {
    return H5Z_JPEGLS;
}
