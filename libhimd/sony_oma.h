#ifndef INCLUDED_LIBHIMD_OMA_H
#define INCLUDED_LIBHIMD_OMA_H

#define EA3_FORMAT_HEADER_SIZE 96

#ifdef __cplusplus
extern "C" {
#endif

struct trackinfo;
void make_ea3_format_header(char * header, const struct trackinfo * trkinfo);

#ifdef __cplusplus
}
#endif

#endif