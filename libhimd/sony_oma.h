#ifndef INCLUDED_LIBHIMD_OMA_H
#define INCLUDED_LIBHIMD_OMA_H

#include "codecinfo.h"

#define EA3_FORMAT_HEADER_SIZE 96

#ifdef __cplusplus
extern "C" {
#endif

void make_ea3_format_header(char * header, const struct sony_codecinfo * ci);

#ifdef __cplusplus
}
#endif

#endif