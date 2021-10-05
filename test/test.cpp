#define CATCH_CONFIG_MAIN

#include <hdf5.h>
#include <catch2/catch.hpp>
#include <Eigen/Core>

#include <filesystem>
#include <iostream>
#include <random>

const int jpegls_filter_id = 32012;

template<typename Scalar>
using Mx = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
namespace fs = std::filesystem;

namespace {
    fs::path temp_file() {
        constexpr std::string_view alphanum = "abcdefghjklmnpqrstuvwxyz0123456789";
        std::random_device dev{};
        std::mt19937 prg{dev()};
        std::string prefix;
        std::sample(alphanum.begin(), alphanum.end(), std::back_inserter(prefix), 8, prg);
        return fs::temp_directory_path() / (prefix + ".h5");
    }

    template<typename Scalar>
    Mx<Scalar> test_data(Eigen::Index rows, Eigen::Index cols) {
        std::mt19937 prg{0};
        std::uniform_int_distribution<int64_t> dist(std::numeric_limits<Scalar>::lowest(), std::numeric_limits<Scalar>::max());
        constexpr Eigen::Index blocks_per_row = 4;
        constexpr Eigen::Index blocks_per_column = 4;
        Eigen::Index row_block_size = rows / blocks_per_row;
        std::array<Eigen::Index, blocks_per_row>
            row_blocks{row_block_size, row_block_size, row_block_size, rows - (blocks_per_row - 1) * row_block_size};

        Eigen::Index column_block_size = cols / blocks_per_column;
        std::array<Eigen::Index, blocks_per_column>
            column_blocks{column_block_size, column_block_size, column_block_size, cols - (blocks_per_column - 1) * column_block_size};

        Mx<Scalar> data(rows, cols);
        for (int j = 0, block_j = 0; j < blocks_per_row; block_j += row_blocks[j], ++j) {
            for (int i = 0, block_i = 0; i < blocks_per_column; block_i += column_blocks[i], ++i) {
                data.block(block_j, block_i, row_blocks[j], column_blocks[i]).array() = dist(prg);
            }
        }

        return data;
    }

    std::pair<H5Z_filter_t, std::string> get_filter_info(hid_t property_list_id) {
        unsigned int flags{};
        size_t num_parameters = 8;
        std::array<unsigned int, 8> parameters{};
        std::array<char, 1024> name{};
        auto filter_id = H5Pget_filter2(
            property_list_id,
            0,
            &flags,
            &num_parameters,
            parameters.data(),
            name.size(),
            name.data(),
            NULL);

        return {filter_id, std::string(name.data())};
    }
}

TEST_CASE("plugin is available", "[plugin]") {
    REQUIRE(H5Zfilter_avail(jpegls_filter_id) != 0);
}

template<typename T>
struct hdf5_type_traits {
    static hid_t type() { throw; }
};

template<>
struct hdf5_type_traits<uint8_t> {
    static hid_t type() { return H5T_STD_U8LE; } };

template<>
struct hdf5_type_traits<int8_t> { static hid_t type() { return H5T_STD_I8LE; } };

template<>
struct hdf5_type_traits<uint16_t> { static hid_t type() { return H5T_STD_U16LE; } };

template<>
struct hdf5_type_traits<int16_t> { static hid_t type() { return H5T_STD_I16LE; } };

void cleanup(hid_t &file_id, hid_t &space_id, hid_t &dset_id, hid_t &dcpl_id) {
    if (dcpl_id >= 0) {
        H5Pclose(dcpl_id);
        dcpl_id = -1;
    }
    if (dset_id >= 0) {
        H5Dclose(dset_id);
        dset_id = -1;
    }
    if (space_id >= 0) {
        H5Sclose(space_id);
        space_id = -1;
    }
    if (file_id >= 0) {
        H5Fclose(file_id);
        file_id = -1;
    }
}

SCENARIO("The filter can only be applied to 2D datasets", "[plugin]") {
    auto file_name = temp_file().string();
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1;
    constexpr auto dataset_name = "test";
    herr_t status;
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hsize_t dim = 4;
    const hsize_t rank = 3;
    std::array<uint8_t, dim * dim * dim> data{};
    GIVEN("A 3D array of integers") {
        std::array<hsize_t, rank> dimensions{dim, dim, dim};
        space_id = H5Screate_simple(dimensions.size(), dimensions.data(), NULL);
        REQUIRE(space_id >= 0);
        WHEN("The array is written to a dataset") {
            THEN("The filter cannot be applied") {
                dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
                REQUIRE(dcpl_id >= 0);
                status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_MANDATORY, 0, NULL);
                REQUIRE(status >= 0);
                std::array<hsize_t, rank> chunk{dim, dim, dim};
                status = H5Pset_chunk(dcpl_id, chunk.size(), chunk.data());
                REQUIRE(status >= 0);
                dset_id = H5Dcreate(file_id, dataset_name, hdf5_type_traits<uint8_t>::type(), space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
                REQUIRE(dset_id >= 0);
                status = H5Dwrite(dset_id, hdf5_type_traits<uint8_t>::type(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
                REQUIRE(status < 0);
            }
        }
    }
    cleanup(file_id, space_id, dset_id, dcpl_id);
}

TEMPLATE_TEST_CASE("Scenario: valid data can written to an HDF5 file, compressed, decompressed and read back", "[plugin][template]", uint8_t, int8_t, uint16_t, int16_t) {
    // Adapted from https://github.com/HDFGroup/hdf5_plugins/blob/master/BZIP2/example/h5ex_d_bzip2.c
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1;
    herr_t status;
    constexpr auto dataset_name = "test";
    auto file_name = temp_file().string();
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(file_id >=0);
    GIVEN("A 2D array of integers") {
        constexpr hsize_t rows = 943;
        constexpr hsize_t cols = 721;
        auto data = test_data<TestType>(rows, cols);
        std::array<hsize_t, 2> dimensions{rows, cols};
        space_id = H5Screate_simple(dimensions.size(), dimensions.data(), NULL);
        REQUIRE(space_id >= 0);
        WHEN("The array is written to a dataset in a single chunk") {
            dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
            REQUIRE(dcpl_id >= 0);
            status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_MANDATORY, 0, NULL);
            REQUIRE(status >= 0);
            std::array<hsize_t, 2> chunk{rows, cols};
            status = H5Pset_chunk(dcpl_id, chunk.size(), chunk.data());
            REQUIRE(status >= 0);
            dset_id = H5Dcreate(file_id, dataset_name, hdf5_type_traits<TestType>::type(), space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
            REQUIRE(dset_id >= 0);
            status = H5Dwrite(dset_id, hdf5_type_traits<TestType>::type(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
            REQUIRE(status >= 0);
            auto uncompressed_size = data.size() * sizeof(TestType);
            THEN("The storage size of the dataset is less than that of the array") {
                auto storage_size = H5Dget_storage_size(dset_id);
                REQUIRE(storage_size > 0);
                auto compression_ratio = static_cast<double>(uncompressed_size) / static_cast<double>(storage_size);
                std::cout << "Compression ratio: " << compression_ratio << "\n";
                REQUIRE(compression_ratio > 100);
            }
            cleanup(file_id, space_id, dset_id, dcpl_id);
            status = H5close();
            REQUIRE(status >= 0);

            AND_WHEN("The dataset is read back") {
                file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                REQUIRE(file_id >= 0);
                dset_id = H5Dopen(file_id, dataset_name, H5P_DEFAULT);
                REQUIRE(dset_id >= 0);
                dcpl_id = H5Dget_create_plist(dset_id);
                REQUIRE(dcpl_id >= 0);
                THEN("The filter has been applied") {
                    const auto [filter_id, filter_name] = get_filter_info(dcpl_id);
                    REQUIRE(filter_id == jpegls_filter_id);
                    std::cout << "Filter info: " << filter_name << "\n";

                    AND_THEN("The data is identical to the original") {
                        Mx<TestType> actual_data(rows, cols);
                        status = H5Dread(dset_id, hdf5_type_traits<TestType>::type(), H5S_ALL, H5S_ALL, H5P_DEFAULT, actual_data.data());
                        REQUIRE(status >= 0);
                        REQUIRE(!(data - actual_data).any());
                    }
                }
            }
        }
        cleanup(file_id, space_id, dset_id, dcpl_id);
    }
}
