#define CATCH_CONFIG_MAIN
#include "hdf5.h"

#include <catch2/catch.hpp>

#include <stdio.h>
#include <stdlib.h>

#define FILE "h5ex_d_bzip2.h5"
#define DATASET "DS1"
#define DIM0 32
#define DIM1 64
#define CHUNK0 16
#define CHUNK1 32
const int jpegls_filter_id = 32012;

TEST_CASE("plugin is available", "[plugin]") {
    REQUIRE(H5Zfilter_avail(jpegls_filter_id) != 0);
}

SCENARIO("data in an hdf5 file can be compressed", "[plugin]") {
    GIVEN("A 2D array of 16bit integers in a checkerboard pattern") {
        WHEN("The array is written to a dataset") {
            THEN("The storage size of the dataset is less than that of the array") {

            }
        }
        AND_WHEN("The dataset is read back") {
            THEN("The data is identical to the original") {

            }
        }
    }
}

TEST_CASE("round trip a compressed dataset", "[h5jpegls]") {
    std::cout << std::getenv("HDF5_PLUGIN_PATH") << std::endl;
    // Adapted from https://github.com/HDFGroup/hdf5_plugins/blob/master/BZIP2/example/h5ex_d_bzip2.c
    hid_t file_id = -1;
    hid_t space_id = -1;
    hid_t dset_id = -1;
    hid_t dcpl_id = -1; /* Handles */
    herr_t status;
    H5Z_filter_t filter_id = 0;
    char filter_name[80];
    hsize_t dims[2] = {DIM0, DIM1}, 
            chunk[2] = {CHUNK0, CHUNK1};
    size_t nelmts = 1;                     /* number of elements in cd_values */
    const unsigned int cd_values[8] = {0, 0, 0, 0, 0, 0, 0, 0}; /* bzip2 default level is 2 */
    unsigned int values_out[8] = {99, 99, 99, 99, 99, 99, 99, 99};
    unsigned int flags;
    htri_t avail;
    unsigned filter_config;
    int16_t wdata[DIM0][DIM1], /* Write buffer */
        rdata[DIM0][DIM1], /* Read buffer */
        max, i, j;

    /*
     * Initialize data.
     */
    for (i = 0; i < DIM0; i++)
        for (j = 0; j < DIM1; j++)
            wdata[i][j] = i * j - j;

    /*
     * Create a new file using the default properties.
     */
    file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        FAIL("Error creating file");
    }

    /*
     * Create dataspace. Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space_id = H5Screate_simple(2, dims, NULL);
    if (space_id < 0) {
        FAIL("Error creating dataspace");
    }

    /*
     * Create the dataset creation property list, add the bzip2
     * compression filter and set the chunk size.
     */
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    if (dcpl_id < 0) {
        FAIL("Error creating dataset properties");
    }
    status = H5Pset_filter(dcpl_id, jpegls_filter_id, H5Z_FLAG_OPTIONAL, 0, NULL);
    if (status < 0) {
        FAIL("Error setting filter");
    }

    /*
     * Check that filter is registered with the library now.
     * If it is registered, retrieve filter's configuration.
     */
    avail = H5Zfilter_avail(jpegls_filter_id);
    if (avail) {
        status = H5Zget_filter_info(jpegls_filter_id, &filter_config);
        if ((filter_config & H5Z_FILTER_CONFIG_ENCODE_ENABLED) && (filter_config & H5Z_FILTER_CONFIG_DECODE_ENABLED))
            printf("bzip2 filter is available for encoding and decoding.\n");
    }
    else {
        FAIL("Filter could not be found");
    }
    status = H5Pset_chunk(dcpl_id, 2, chunk);
    if (status < 0) {
        FAIL("Error setting chunk size");
    }

    /*
     * Create the dataset.
     */
    printf("....Writing bzip2 compressed data ................\n");
    dset_id = H5Dcreate(file_id, DATASET, H5T_STD_I16LE, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
    if (dset_id < 0) {
        FAIL("Error creating dataset");
    }

    /*
     * Write the data to the dataset.
     */
    status = H5Dwrite(dset_id, H5T_STD_I16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if (status < 0) {
        FAIL("Error writing to dataset");
    }

    /*
     * Close and release resources.
     */
    H5Dclose(dset_id);
    dset_id = -1;
    H5Pclose(dcpl_id);
    dcpl_id = -1;
    H5Sclose(space_id);
    space_id = -1;
    H5Fclose(file_id);
    file_id = -1;
    status = H5close();
    if (status < 0) {
        FAIL("Error closing library");
    }

    printf("....Close the file and reopen for reading ........\n");
    /*
     * Now we begin the read section of this example.
     */

    /*
     * Open file and dataset using the default properties.
     */
    file_id = H5Fopen(FILE, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        FAIL("Error opening file");
    }

    dset_id = H5Dopen(file_id, DATASET, H5P_DEFAULT);
    if (dset_id < 0) {
        FAIL("Error opening dataset");
    }

    /*
     * Retrieve dataset creation property list.
     */
    dcpl_id = H5Dget_create_plist(dset_id);
    if (dcpl_id < 0) {
        FAIL("Error getting dataset property list");
    }
    /*
    * Retrieve and print the filter id, compression level and filter's name
   for bzip2.
    */
    filter_id = H5Pget_filter2(dcpl_id, (unsigned)0, &flags, &nelmts, values_out, sizeof(filter_name), filter_name, NULL);
    printf("Filter info is available from the dataset creation property \n ");
    printf(" Filter identifier is ");
    switch (filter_id) {
    case jpegls_filter_id:
        printf("%d\n", filter_id);
        printf(" Number of parameters is %d with the value %u\n", nelmts, values_out[0]);
        printf(" To find more about the filter check %s\n", filter_name);
        break;
    default:
        printf("Not expected filter\n");
        break;
    }

    /*
     * Read the data using the default properties.
     */
    printf("....Reading bzip2 compressed data ................\n");
    status = H5Dread(dset_id, H5T_STD_I16LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rdata[0]);
    if (status < 0) {
        FAIL("Error reading data");
    }
    /*
     * Find the maximum value in the dataset, to verify that it was
     * read correctly.
     */
    max = rdata[0][0];
    for (i = 0; i < DIM0; i++)
        for (j = 0; j < DIM1; j++) {
            /*printf("%d \n", rdata[i][j]); */
            if (max < rdata[i][j])
                max = rdata[i][j];
        }
    /*
     * Print the maximum value.
     */
    printf("Maximum value in %s is %d\n", DATASET, max);
    /*
     * Check that filter is registered with the library now.
     */
    avail = H5Zfilter_avail(jpegls_filter_id);
    if (avail)
        printf("bzip2 filter is available now since H5Dread triggered loading of the filter.\n");

    /*
     * Close and release resources.
     */
    if (dcpl_id >= 0) {
        H5Pclose(dcpl_id);
    }
    if (dset_id >= 0) {
        H5Dclose(dset_id);
    }
    if (space_id >= 0) {
        H5Sclose(space_id);
    }
    if (file_id >= 0) {
        H5Fclose(file_id);
    }

    SUCCEED();
}