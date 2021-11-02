#ifndef H5JPEGLS_H
#define H5JPEGLS_H

#include <H5PLextern.h>

#ifdef __cplusplus
namespace h5jpegls {
    void register_plugin();
}
extern "C" {
#endif
void h5jpegls_register_plugin(void);
#ifdef __cplusplus
}
#endif

#endif // H5JPEGLS_H
