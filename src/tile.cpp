#include "tile.hpp"

h5jpegls::Tile h5jpegls::copy_tile_from_buffer(
    const void * buffer, int image_cols, uint16_t row, uint16_t col, const charls::frame_info & frame) {
    auto bytes_per_sample = frame.bits_per_sample / 8;
    auto src_stride = image_cols * frame.component_count * bytes_per_sample;
    auto col_stride = frame.component_count * bytes_per_sample;
    auto src_offset = row * src_stride + col * col_stride;

    auto dest_stride = frame.width * frame.component_count * bytes_per_sample;
    Tile tile{};
    tile.row = row;
    tile.col = col;
    tile.data.resize(frame.height * dest_stride);

    for (int j = 0; j < frame.height; ++j) {
        auto dest_row_offset = j * dest_stride;
        auto src_row_offset = src_offset + j * src_stride;
        memcpy(
            tile.data.data() + dest_row_offset,
            static_cast<const uint8_t *>(buffer) + src_row_offset,
            dest_stride
        );
    }

    return tile;
}
