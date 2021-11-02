#include <h5jpegls/h5jpegls.h>
#include "version.hpp"

#include <H5Zpublic.h>
#pragma warning( push )
#pragma warning( disable : 4768 )
#include <hdf5.h>
#pragma warning( pop )
#include <charls/charls.h>

#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
const H5Z_filter_t H5Z_FILTER_JPEGLS = 32012;

struct LegacyCodecParameters {
    static LegacyCodecParameters from_array(const unsigned int params[]) {
        return {.width = params[0], .height = params[1], .bytes_per_pixel = params[2]};
    }

    unsigned int width{};
    unsigned int height{};
    unsigned int bytes_per_pixel{};
};

struct CodecParameters {
    static CodecParameters from_array(const unsigned int params[]) {
        return { .chunk_m = params[0], .chunk_n = params[1], .bits_per_element = params[2], .num_components = params[3] };
    }

    unsigned int chunk_m{};
    unsigned int chunk_n{};
    unsigned int bits_per_element{};
    unsigned int num_components = 1;

    [[nodiscard]] std::array<unsigned int, 4> elements() const {
        return {
            chunk_m, chunk_n, bits_per_element, num_components
        };
    }
};

size_t decode(void **buffer, size_t *buffer_size, size_t data_size, const CodecParameters &) {
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

size_t decode_legacy(void **buffer, size_t *buffer_size, const LegacyCodecParameters &parameters) {
    auto slices = std::min(parameters.height, 24U);
    std::vector<uint32_t> slice_sizes(slices);
    size_t header_size = slices * sizeof(uint32_t);
    std::memcpy(slice_sizes.data(), *buffer, header_size);
    auto pData = static_cast<uint8_t *>(*buffer) + header_size;
    size_t offset = 0;
    size_t required_bytes = parameters.width * parameters.height * parameters.bytes_per_pixel;
    std::vector<uint8_t> combined_buffer(required_bytes);
    auto destination = combined_buffer.begin();
    for (const auto slice : slice_sizes) {
        charls::jpegls_decoder decoder;
        decoder.source(pData + offset, slice);
        decoder.read_header();
        std::vector<uint8_t> decoding_buffer(decoder.destination_size());
        decoder.decode(decoding_buffer);
        std::copy(decoding_buffer.begin(), decoding_buffer.end(), destination);
        std::advance(destination, decoding_buffer.size());
        offset += slice;
    }
    *buffer = std::realloc(*buffer, required_bytes);
    *buffer_size = required_bytes;
    std::memcpy(*buffer, combined_buffer.data(), required_bytes);

    return required_bytes;
}

size_t encode(void **buffer, size_t *buffer_size, size_t data_size, const CodecParameters &parameters) {
    charls::jpegls_encoder encoder;
    charls::frame_info frame{
        .width = parameters.chunk_n,
        .height = parameters.chunk_m,
        .bits_per_sample = static_cast<int32_t>(parameters.bits_per_element),
        .component_count = static_cast<int32_t>(parameters.num_components)
    };
    encoder.frame_info(frame);
    if (frame.component_count > 1) {
        encoder.interleave_mode(charls::interleave_mode::Sample);
    }
    std::vector<uint8_t> encoding_buffer(*buffer_size);
    encoder.destination(encoding_buffer);
    auto num_encoded_bytes = encoder.encode(*buffer, data_size);
    assert(num_encoded_bytes < *buffer_size);
    *buffer_size = num_encoded_bytes;
    memcpy(*buffer, encoding_buffer.data(), num_encoded_bytes);
    return num_encoded_bytes;
}

htri_t can_apply_filter(hid_t dcpl_id, hid_t type_id, hid_t) {
    constexpr hsize_t max_rank = 32;
    std::array<hsize_t, max_rank> chunk_dimensions{};
    auto rank = H5Pget_chunk(dcpl_id, 32, chunk_dimensions.data());
    if (rank != 2 && rank != 3) {
        return 0;
    }
    auto type_class = H5Tget_class(type_id);
    bool can_filter = type_class == H5T_INTEGER;
    if (type_class == H5T_ARRAY) {
        auto base_class = H5Tget_super(type_id);
        can_filter = base_class == H5T_INTEGER;
        H5Tclose(base_class);
    }
    if (!can_filter) {
        return 0;
    }

    auto type_size_in_bytes = H5Tget_size(type_id);
    can_filter = type_size_in_bytes <= 2;
    return can_filter ? 1 : 0;
}

size_t codec_filter(unsigned int flags, size_t num_parameters, const unsigned int parameters[], size_t nbytes, size_t *buf_size,
                    void **buf) {

    try {
        if (flags & H5Z_FLAG_REVERSE) {
            if (num_parameters == 4) {
                return decode(buf, buf_size, nbytes, CodecParameters::from_array(parameters));
            }
            return decode_legacy(buf, buf_size, LegacyCodecParameters::from_array(parameters));
        }
        return encode(buf, buf_size, nbytes, CodecParameters::from_array(parameters));
    } catch (const std::exception &ex) {
        fprintf(stderr, "%s\n", ex.what());
    } catch (...) {
        fprintf(stderr, "Unknown error\n");
    }
    return 0;
}

herr_t h5jpegls_set_local(hid_t dcpl_id, hid_t type_id, hid_t) {
    unsigned int flags;
    size_t num_user_parameters = 8;
    std::array<unsigned int, 8> user_parameters{};
    herr_t status =
        H5Pget_filter_by_id(dcpl_id, H5Z_FILTER_JPEGLS, &flags, &num_user_parameters, user_parameters.data(), 0, nullptr, nullptr);

    if (status < 0) {
        return -1;
    }

    std::array<hsize_t, 32> chunk_dimensions{};
    int rank = H5Pget_chunk(dcpl_id, static_cast<int>(chunk_dimensions.size()), chunk_dimensions.data());
    if (rank < 0) {
        return -1;
    }

    CodecParameters parameters{
        .chunk_m = static_cast<unsigned int>(chunk_dimensions[0]),
        .chunk_n = static_cast<unsigned int>(chunk_dimensions[1]),
    };
    if (rank == 2) {
        parameters.num_components = 1;
    } else {
        parameters.num_components = static_cast<unsigned int>(chunk_dimensions[2]);
    }

    if (num_user_parameters > 0) {
        parameters.bits_per_element = user_parameters[0];
    } else {
        H5T_class_t type_class = H5Tget_class(type_id);
        auto bytes_per_element = static_cast<unsigned int>(H5Tget_size(type_id));
        if (type_class == H5T_ARRAY) {
            hid_t super_type = H5Tget_super(type_id);
            bytes_per_element = static_cast<unsigned int>(H5Tget_size(super_type));
            H5Tclose(super_type);
        }
        if (bytes_per_element == 0) {
            return -1;
        }
        parameters.bits_per_element = bytes_per_element * 8;
    }

    auto cd_values = parameters.elements();
    status = H5Pmodify_filter(dcpl_id, H5Z_FILTER_JPEGLS, flags, cd_values.size(), cd_values.data());

    if (status < 0) {
        return -1;
    }

    return 1;
}
}

extern "C" const H5Z_class2_t H5Z_JPEGLS[1] = {{
    H5Z_CLASS_T_VERS,                 /* H5Z_class_t version */
    (H5Z_filter_t)H5Z_FILTER_JPEGLS,         /* Filter id number */
    1,              /* encoder_present flag (set to true) */
    1,              /* decoder_present flag (set to true) */
    h5jpegls::description().data(),
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

void h5jpegls::register_plugin() {
    h5jpegls_register_plugin();
}

void h5jpegls_register_plugin() {
    H5Zregister(H5Z_JPEGLS);
}
