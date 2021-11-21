#include "../src/tile.hpp"

#include <charls/charls.h>
#include <catch2/catch.hpp>

TEMPLATE_TEST_CASE("Senario: a tile can be copied from the source buffer", "[template][tile]", uint8_t, int8_t, uint16_t, int16_t) {
    std::vector<TestType> image{
        0, 1, 2, 3, 4, 5, 6, 7,
        10, 11, 12, 13, 14, 15, 16, 17,
        20, 21, 22, 23, 24, 25, 26, 27,
        30, 31, 32, 33, 34, 35, 36, 37,
        40, 41, 42, 43, 44, 45, 46, 47,
        50, 51, 52, 53, 54, 55, 56, 57
    };

    constexpr uint16_t row = 1;
    constexpr uint16_t col = 2;
    constexpr int cols = 8;
    charls::frame_info frame;
    frame.width = 4;
    frame.height = 3;
    frame.bits_per_sample = sizeof(TestType) * 8;
    frame.component_count = 1;
    auto tile = h5jpegls::copy_tile_from_buffer(image.data(), cols, row, col, frame);
    std::vector<TestType> expected_tile_data{
        12, 13, 14, 15,
        22, 23, 24, 25,
        32, 33, 34, 35
    };
    std::vector<TestType> tile_data(expected_tile_data.size());
    memcpy(tile_data.data(), tile.data.data(), tile_data.size() * sizeof(TestType));

    REQUIRE(expected_tile_data == tile_data);
}
