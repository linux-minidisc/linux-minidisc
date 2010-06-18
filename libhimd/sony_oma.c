#include "himd.h"
#include "sony_oma.h"
#include <string.h>

void make_ea3_format_header(char * header, const struct trackinfo * trkinfo)
{
    static const char ea3header[12] =
        {0x45, 0x41, 0x33, 0x01, 0x00, 0x60, 
         0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

    memset(header, 0, EA3_FORMAT_HEADER_SIZE);
    memcpy(header   , ea3header,12);
    /* Do not set the content ID - this activates DRM stuff in Sonic Stage.
       A track with an unknown content ID can not be converted nor transferred.
       A zero content ID seems to mean "no DRM, for real!" */
    /*memcpy(header+12, trkinfo->contentid,20);*/
    header[32] = trkinfo->codec_id;
    memcpy(header+33, trkinfo->codecinfo, 3);
}
