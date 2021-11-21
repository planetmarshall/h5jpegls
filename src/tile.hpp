#ifndef H5JPEGLS_TILE_HPP
#define H5JPEGLS_TILE_HPP

#include <charls/charls.h>

#include <cstdint>
#include <vector>


namespace h5jpegls {
    struct Tile {
        uint16_t row;
        uint16_t col;
        std::vector<uint8_t> data;
    };

    Tile copy_tile_from_buffer(const void * buffer, int buffer_cols, uint16_t row, uint16_t col, const charls::frame_info & frame);
}

#endif // H5JPEGLS_TILE_HPP
