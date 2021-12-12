#ifndef H5JPEGLS_H
#define H5JPEGLS_H

#include <H5PLextern.h>

#ifdef __cplusplus
namespace h5jpegls {
    static constexpr H5Z_filter_t filter_id = 32012;
    void register_plugin();
}
extern "C" {
#endif
#define H5JPEGLS_FILTER_ID 32012
void h5jpegls_register_plugin(void);
#ifdef __cplusplus
}
#endif

#endif // H5JPEGLS_H
