#define CATCH_CONFIG_MAIN
#ifdef H5JPEGLS_BUILD_LIBRARY
#include <h5jpegls/h5jpegls.h>
#endif

#pragma warning( push )
#pragma warning( disable : 4768 )
#include <hdf5.h>
#pragma warning( pop )
#include <catch2/catch.hpp>

#include <filesystem>
#include <iostream>
#include <random>
#include <vector>
#include <array>

const int jpegls_filter_id = 32012;
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
    std::vector<Scalar> test_data(size_t rows, size_t cols, size_t channels) {
        std::vector<Scalar> data(rows * cols * channels);
        for ( size_t j = 0; j < rows; ++j) {
            size_t row_offset = j * cols;
            for ( size_t i = 0; i < cols; ++i ) {
                size_t col_offset = row_offset + (i * channels);
                for (size_t p = 0; p < channels; ++p) {
                    data[col_offset + p] = static_cast<Scalar>((col_offset + p) % std::numeric_limits<Scalar>::max());
                }
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

void register_plugin() {
#ifdef H5JPEGLS_BUILD_LIBRARY
    h5jpegls_register_plugin();
#endif
    }
}

TEST_CASE("plugin is available", "[plugin]") {
    register_plugin();
    REQUIRE(H5Zfilter_avail(jpegls_filter_id) != 0);
}

template<typename T>
struct hdf5_type_traits { static hid_t type() { throw; } };
template<>
struct hdf5_type_traits<uint8_t> { static hid_t type() { return H5T_STD_U8LE; } };
template<>
struct hdf5_type_traits<int8_t> { static hid_t type() { return H5T_STD_I8LE; } };
template<>
struct hdf5_type_traits<uint16_t> { static hid_t type() { return H5T_STD_U16LE; } };
template<>
struct hdf5_type_traits<int16_t> { static hid_t type() { return H5T_STD_I16LE; } };
template<>
struct hdf5_type_traits<int32_t> { static hid_t type() { return H5T_STD_I32LE; } };
template<>
struct hdf5_type_traits<uint32_t> { static hid_t type() { return H5T_STD_U32LE; } };
template<>
struct hdf5_type_traits<int64_t> { static hid_t type() { return H5T_STD_I64LE; } };
template<>
struct hdf5_type_traits<uint64_t> { static hid_t type() { return H5T_STD_U64LE; } };
template<>
struct hdf5_type_traits<float> { static hid_t type() { return H5T_IEEE_F32LE; } };
template<>
struct hdf5_type_traits<double> { static hid_t type() { return H5T_IEEE_F64LE; } };

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

TEMPLATE_TEST_CASE("Scenario: the filter can only be applied to compatible integer valued datasets", "[plugin][template]", float, double, int32_t, uint64_t) {
    register_plugin();
    auto file_name = temp_file().string();
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1;
    constexpr auto dataset_name = "test";
    herr_t status;
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hsize_t dim = 4;
    const hsize_t rank = 2;
    GIVEN("A 2D dataset") {
        std::array<hsize_t, rank> dimensions{dim, dim};
        space_id = H5Screate_simple(static_cast<int>(dimensions.size()), dimensions.data(), NULL);
        REQUIRE(space_id >= 0);
        WHEN("The dataset is created of 32 bit integer type") {
            THEN("The filter cannot be applied") {
                dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
                REQUIRE(dcpl_id >= 0);
                status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_MANDATORY, 0, NULL);
                REQUIRE(status >= 0);
                std::array<hsize_t, rank> chunk{dim, dim};
                status = H5Pset_chunk(dcpl_id, static_cast<int>(chunk.size()), chunk.data());
                REQUIRE(status >= 0);
                dset_id = H5Dcreate(file_id, dataset_name, hdf5_type_traits<TestType>::type(), space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
                REQUIRE(dset_id < 0);
            }
        }
    }
    cleanup(file_id, space_id, dset_id, dcpl_id);
}

SCENARIO("The filter can only be applied to 2D or 3D datasets", "[plugin]") {
    register_plugin();
    auto file_name = temp_file().string();
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1;
    constexpr auto dataset_name = "test";
    herr_t status;
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hsize_t dim = 4;
    const hsize_t rank = 4;
    GIVEN("A 4D dataset") {
        std::array<hsize_t, rank> dimensions{dim, dim, dim, dim};
        space_id = H5Screate_simple(static_cast<int>(dimensions.size()), dimensions.data(), NULL);
        REQUIRE(space_id >= 0);
        WHEN("The dataset is created") {
            THEN("The filter cannot be applied") {
                dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
                REQUIRE(dcpl_id >= 0);
                status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_MANDATORY, 0, NULL);
                REQUIRE(status >= 0);
                std::array<hsize_t, rank> chunk{dim, dim, dim, dim};
                status = H5Pset_chunk(dcpl_id, static_cast<int>(chunk.size()), chunk.data());
                REQUIRE(status >= 0);
                dset_id = H5Dcreate(file_id, dataset_name, hdf5_type_traits<uint8_t>::type(), space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
                REQUIRE(dset_id < 0);
            }
        }
    }
    cleanup(file_id, space_id, dset_id, dcpl_id);
}

template<typename TestType, size_t Rank, size_t NumParams>
void roundtrip_test(
    const std::array<hsize_t, Rank> & dimensions,
    const std::array<hsize_t, Rank> & chunk,
    const std::array<unsigned int, NumParams> & parameters
    ) {
    // Adapted from https://github.com/HDFGroup/hdf5_plugins/blob/master/BZIP2/example/h5ex_d_bzip2.c
    register_plugin();
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1;
    herr_t status;
    constexpr auto dataset_name = "test";
    auto file_name = temp_file().string();
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(file_id >=0);
    auto rows = dimensions[0];
    auto cols = dimensions[1];
    auto channels = Rank == 3 ? dimensions[2] : 1;
    auto data = test_data<TestType>(rows, cols, channels);
    space_id = H5Screate_simple(static_cast<int>(dimensions.size()), dimensions.data(), NULL);
    REQUIRE(space_id >= 0);
    WHEN("The array is written to a dataset in a single chunk") {
        dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
        REQUIRE(dcpl_id >= 0);
        status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_MANDATORY, parameters.size(), parameters.data());
        REQUIRE(status >= 0);
        status = H5Pset_chunk(dcpl_id, static_cast<int>(chunk.size()), chunk.data());
        REQUIRE(status >= 0);
        dset_id = H5Dcreate(file_id, dataset_name, hdf5_type_traits<TestType>::type(), space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
        REQUIRE(dset_id >= 0);
        status = H5Dwrite(dset_id, hdf5_type_traits<TestType>::type(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
        REQUIRE(status >= 0);
        double uncompressed_size = static_cast<double>(data.size()) * sizeof(TestType);
        auto storage_size = static_cast<double>(H5Dget_storage_size(dset_id));
        REQUIRE(storage_size > 0);
        auto compression_ratio = uncompressed_size / storage_size;
        std::cout << "Compression ratio: " << compression_ratio << "\n";
        cleanup(file_id, space_id, dset_id, dcpl_id);
        status = H5close();
        REQUIRE(status >= 0);

        register_plugin();
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
                    std::vector<TestType> actual_data(rows * cols * channels);
                    status = H5Dread(dset_id, hdf5_type_traits<TestType>::type(), H5S_ALL, H5S_ALL, H5P_DEFAULT, actual_data.data());
                    REQUIRE(status >= 0);
                    REQUIRE(data == actual_data);
                }
            }
        }
    }
}

TEMPLATE_TEST_CASE("Scenario: valid data can written to an HDF5 file, compressed, decompressed and read back", "[plugin][template]", uint8_t, int8_t, uint16_t, int16_t) {
    GIVEN("A small 2D array of integers in a single chunk") {
        constexpr hsize_t rows = 3;
        constexpr hsize_t cols = 4;
        auto data = test_data<TestType>(rows, cols, 1);
        std::array<hsize_t, 2> dimensions{rows, cols};
        std::array<unsigned int, 0> params{};
        roundtrip_test<TestType>(dimensions, dimensions, params);
    }
    GIVEN("A 2D array of integers in a single chunk") {
        constexpr hsize_t rows = 943;
        constexpr hsize_t cols = 721;
        auto data = test_data<TestType>(rows, cols, 1);
        std::array<hsize_t, 2> dimensions{rows, cols};
        std::array<unsigned int, 0> params{};
        roundtrip_test<TestType>(dimensions, dimensions, params);
    }
    GIVEN("A 3D array of integers in a single chunk") {
        constexpr hsize_t rows = 29;
        constexpr hsize_t cols = 13;
        constexpr hsize_t channels = 3;
        auto data = test_data<TestType>(rows, cols, channels);
        std::array<hsize_t, 3> dimensions{rows, cols, channels};
        std::array<unsigned int, 0> params{};
        roundtrip_test<TestType>(dimensions, dimensions, params);
    }
}
