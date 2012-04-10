/*
 * sony_oma.c
 *
 * This file is part of libhimd, a library for accessing Sony HiMD devices.
 *
 * Copyright (C) 2009-2011 Michael Karcher
 * Copyright (C) 2011 Mårten Cassel
 * Copyright (C) 2011 Thomas Arp
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "himd.h"
#include "sony_oma.h"
#include <string.h>

void make_ea3_format_header(char * header, const struct sony_codecinfo * trkinfo)
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
