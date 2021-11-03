#include <h5jpegls/h5jpegls.h>
#include <hdf5.h>

#include <iostream>

int main() {
    constexpr int filter_id = 32012;
    h5jpegls::register_plugin();
    auto filter_available = H5Zfilter_avail(filter_id);
    if (filter_available <= 0 ) {
        std::cerr << "JPEG-LS Filter not available" << std::endl;
        return -1;
    }
    std::cerr << "JPEG-LS Filter available" << std::endl;

    return 0;
}
