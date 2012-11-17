/*
 * mp3tools.c
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

#include <id3tag.h>
#include "himd.h"

/*
 * gets artist, title and album info from an ID3 tag.
 * The output strings are to be free()d.
 * Returns -1, if id3 informations could be extracted.
 */
int himd_get_songinfo(const char *filepath, char ** artist, char ** title, char **album, struct himderrinfo * status)
{
    struct id3_file * file;
    struct id3_frame const *frame;
    struct id3_tag *tag;
    union id3_field const *field;

    file = id3_file_open(filepath, ID3_FILE_MODE_READONLY);

    tag = id3_file_tag(file);
    if(!tag)
    {
        id3_file_close(file);
        set_status_printf(status, HIMD_ERROR_NO_ID3_TAGS_FOUND, "no id3 tags found in file '%s'", filepath);
        return -1;
    }

    frame = id3_tag_findframe (tag, ID3_FRAME_ARTIST, 0);
    if(frame && (field = &frame->fields[1]) &&
                 id3_field_getnstrings(field) > 0)
        *artist = id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *artist = NULL;

    frame = id3_tag_findframe (tag, ID3_FRAME_TITLE, 0);
    if(frame && (field = &frame->fields[1]) &&
                 id3_field_getnstrings(field) > 0)
        *title = id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *title = NULL;

    frame = id3_tag_findframe (tag, ID3_FRAME_ALBUM, 0);
    if(frame && (field = &frame->fields[1]) &&
                 id3_field_getnstrings(field) > 0)
        *album = id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *album = NULL;

    id3_file_close(file);
    return 0;
}
