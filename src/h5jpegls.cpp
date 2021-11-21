#include <h5jpegls/h5jpegls.h>
#include "version.hpp"

#include <H5Zpublic.h>
#pragma warning( push )
#pragma warning( disable : 4768 )
#include <hdf5.h>
#pragma warning( pop )
#include <charls/charls.h>
#include <tbb/tbb.h>

#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
struct LegacyCodecParameters {
    static LegacyCodecParameters from_array(const unsigned int params[]) {
        return {
            .width = params[0],
            .height = params[1],
            .bytes_per_pixel = params[2]
        };
    }

    unsigned int width{};
    unsigned int height{};
    unsigned int bytes_per_pixel{};
};

struct CodecParameters {
    static CodecParameters from_array(const unsigned int params[]) {
        return {
            .chunk_m = params[0],
            .chunk_n = params[1],
            .bits_per_element = params[2],
            .num_components = params[3],
            .blocks_m = params[4],
            .blocks_n = params[5]
        };
    }

    unsigned int chunk_m{};
    unsigned int chunk_n{};
    unsigned int bits_per_element{};
    unsigned int num_components = 1;
    unsigned int blocks_m = 1;
    unsigned int blocks_n = 1;

    [[nodiscard]] std::array<unsigned int, 6> elements() const {
        return {
            chunk_m, chunk_n, bits_per_element, num_components, blocks_m, blocks_n
        };
    }
};

size_t decode_blocks() {

}

size_t decode(void **buffer, size_t *buffer_size, size_t data_size, const CodecParameters &params) {

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

std::vector<uint8_t> get_tile_for_encoding(
    const uint8_t * buffer,
    const int current_row,
    const int current_col,
    const int total_cols,
    const charls::frame_info & frame) {
    auto bytes_per_sample = frame.bits_per_sample / 8;
    auto src_stride = total_cols * frame.component_count * bytes_per_sample;
    auto col_stride = frame.component_count * bytes_per_sample;
    auto src_offset = current_row * src_stride + current_col * col_stride;

    auto dest_stride = frame.width * frame.component_count * bytes_per_sample;
    std::vector<uint8_t> tile(frame.height * dest_stride);

    for (int j = 0; j < frame.height; ++j) {
        auto dest_row_offset = j * dest_stride;
        auto src_row_offset = src_offset + j * src_stride;
        memcpy(tile.data() + dest_row_offset, buffer + src_row_offset, dest_stride);
    }

    return tile;
}

size_t encode_tiles(void *buffer, const CodecParameters & params) {
    auto row_grain = params.chunk_m / params.blocks_m;
    auto col_grain = params.chunk_n / params.blocks_n;
    auto blocked_range = tbb::blocked_range2d<uint32_t , uint32_t>(
        0, params.chunk_m, row_grain,
        0, params.chunk_n, col_grain
    );
    tbb::parallel_for(blocked_range, [&] ( const auto & range) {
      uint32_t width = range.cols().end() - range.cols().begin();
      uint32_t height = range.rows().end() - range.rows().begin();
      charls::jpegls_encoder encoder{};
      charls::frame_info frame {
          .width = width,
          .height = height,
          .bits_per_sample = static_cast<int32_t>(params.bits_per_element),
          .component_count = static_cast<int32_t>(params.num_components),
        };
      encoder.frame_info(frame);
      if (frame.component_count > 1) {
          encoder.interleave_mode(charls::interleave_mode::Sample);
      }
      std::vector<uint8_t> encoding_buffer(encoder.estimated_destination_size());
      encoder.destination(encoding_buffer);
      auto src_buffer = get_tile_for_encoding(
          static_cast<uint8_t *>(buffer),
          range.rows().begin(),
          range.cols().begin(),
          params.chunk_n,
          frame);

      auto num_encoded_bytes = encoder.encode(src_buffer);
    });

    return 0;
}

size_t encode(void **buffer, size_t *buffer_size, size_t data_size, const CodecParameters &parameters) {
    if (parameters.blocks_m != 1 || parameters.blocks_n != 1) {
        return encode_tiles(*buffer, parameters);
    }

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
    auto destination_size = encoder.estimated_destination_size();
    if (*buffer_size < destination_size) {
        *buffer = std::realloc(*buffer, destination_size);
        *buffer_size = destination_size;
    }
    std::vector<uint8_t> encoding_buffer(*buffer_size);
    encoder.destination(encoding_buffer);
    auto num_encoded_bytes = encoder.encode(*buffer, data_size);
    memcpy(*buffer, encoding_buffer.data(), num_encoded_bytes);
    return num_encoded_bytes;
}

htri_t can_apply_filter(hid_t dcpl_id, hid_t type_id, hid_t) {
    constexpr hsize_t max_rank = 32;
    std::array<hsize_t, max_rank> chunk_dimensions{};
    auto rank = H5Pget_chunk(dcpl_id, max_rank, chunk_dimensions.data());
    if (rank != 2 && rank != 3) {
        return 0;
    }
    auto type_class = H5Tget_class(type_id);
    bool can_filter = type_class == H5T_INTEGER || type_class == H5T_ENUM;
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
            if (num_parameters > 3) {
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
        H5Pget_filter_by_id(dcpl_id, h5jpegls::filter_id, &flags, &num_user_parameters, user_parameters.data(), 0, nullptr, nullptr);

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

    if (num_user_parameters == 0 || user_parameters[0] == 0) {
        H5T_class_t type_class = H5Tget_class(type_id);
        auto bytes_per_element = static_cast<unsigned int>(H5Tget_size(type_id));
        if (type_class == H5T_ARRAY || type_class == H5T_ENUM) {
            hid_t super_type = H5Tget_super(type_id);
            bytes_per_element = static_cast<unsigned int>(H5Tget_size(super_type));
            H5Tclose(super_type);
        }
        if (bytes_per_element == 0) {
            return -1;
        }
        parameters.bits_per_element = bytes_per_element * 8;
    } else {
        parameters.bits_per_element = user_parameters[0];
    }
    if (num_user_parameters == 3) {
        parameters.blocks_m = std::min(user_parameters[1], parameters.chunk_m);
        parameters.blocks_n = std::min(user_parameters[2], parameters.chunk_n);
    }

auto cd_values = parameters.elements();
    status = H5Pmodify_filter(dcpl_id, h5jpegls::filter_id, flags, cd_values.size(), cd_values.data());

    if (status < 0) {
        return -1;
    }

    return 1;
}
}

extern "C" const H5Z_class2_t H5Z_JPEGLS[1] = {{
    H5Z_CLASS_T_VERS,                 /* H5Z_class_t version */
    h5jpegls::filter_id,
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
