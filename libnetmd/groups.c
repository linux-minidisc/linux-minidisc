/*
 * groups.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 * Copyright (C) 2002, 2003 Marc Britten
 * Copyright (C) 2011 Alexander Sulfrian
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

#include "groups.h"

#include "libnetmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


netmd_error
netmd_set_group_title(netmd_dev_handle *dev, minidisc *md, netmd_group_id group, const char *title)
{
    free(md->groups[group].name);
    md->groups[group].name = strdup(title);

    return netmd_write_disc_header(dev, md);
}

static void
netmd_append_group(minidisc *md, const char *name, int first, int last)
{
    md->group_count++;
    md->groups = realloc(md->groups, sizeof(struct netmd_group) * md->group_count);

    netmd_group_t *group = &md->groups[md->group_count - 1];

    group->name = strdup(name);
    group->first = first;
    group->last = last;
}

/* So far we've encountered the following first segments in the disc title:
 *
 * 0[-n];<title>// - titled disc.
 * <title>// - titled disc
 * 0[-n];// - untitled disc
 * n{n>0};<group>// - untitled disc, group with one track
 * n{n>0}-n2{n2>n>0};group// - untitled disc, group with multiple tracks
 * ;group// - untitled disc, group with no tracks
 */

static void
netmd_parse_group(minidisc *md, char *group)
{
    char *sep = strchr(group, ';');

    char *title = group;

    unsigned int first = 0;
    unsigned int last = 0;

    if (sep != NULL) {
        title = sep + 1;

        *sep = '\0';
        int res = sscanf(group, "%u-%u", &first, &last);
        if (res != 2) {
            res = sscanf(group, "%u", &first);
            if (res == 1) {
                last = first;
            } else {
                first = last = 0;
            }
        }
        *sep = ';';
    }

    netmd_log(NETMD_LOG_DEBUG, "Parsed group: '%s' -> first=%u, last=%u, title='%s'\n", group, first, last, title);
    netmd_append_group(md, title, first, last);
}

static void
netmd_parse_disc_title(minidisc *md, char *title)
{
    size_t title_length = strlen(title);

    char *cur = title;
    char *end = title + title_length;

    while (*cur && cur < end) {
        char *delim = strstr(cur, "//");
        if (delim == NULL) {
            // No more group data
            break;
        }

        *delim = '\0';
        netmd_parse_group(md, cur);
        *delim = '/';

        cur = delim + 2;
    }

    if (md->group_count == 0) {
        // No group data found, title is disc title
        netmd_append_group(md, cur, 0, 0);
    }
}

minidisc *
netmd_minidisc_load(netmd_dev_handle *devh)
{
    minidisc *md = calloc(1, sizeof(minidisc));

    char disc[7*255 + 1];

    if (netmd_get_disc_title(devh, disc, sizeof(disc)) > 0) {
        netmd_parse_disc_title(md, disc);
    } else {
        // Create empty disc title on error
        netmd_parse_disc_title(md, "");
    }

    return md;
}

void
netmd_minidisc_free(minidisc *md)
{
    for (unsigned int i = 0; i < md->group_count; i++) {
        free(md->groups[i].name);
    }

    free(md->groups);
    free(md);
}

netmd_error
netmd_create_group(netmd_dev_handle *dev, minidisc *md, const char *name)
{
    netmd_append_group(md, name, 0, 0);

    return netmd_write_disc_header(dev, md);
}

netmd_error
netmd_delete_group(netmd_dev_handle *dev, minidisc *md, netmd_group_id group)
{
    if (group < 1 || group >= md->group_count) {
        return NETMD_GROUP_DOES_NOT_EXIST;
    }

    free(md->groups[group].name);
    for (int i=group; i<md->group_count-1; ++i) {
        md->groups[i] = md->groups[i+1];
    }

    md->group_count--;

    return netmd_write_disc_header(dev, md);
}

static netmd_error
netmd_generate_disc_title(const minidisc *md, char *title, size_t title_length)
{
    char *write_ptr = title;
    char *end_ptr = title + title_length;

    int written;

    for (int i=0; i<md->group_count && write_ptr < end_ptr; ++i) {
        netmd_group_t *group = &md->groups[i];

        if (i == 0) {
            // disc title
            written = snprintf(write_ptr, (intptr_t)(end_ptr - write_ptr), "0");
            if (written <= 0) {
                return NETMD_ERROR;
            }
            write_ptr += written;
        } else if (group->first != 0) {
            // non-empty group
            written = snprintf(write_ptr, (intptr_t)(end_ptr - write_ptr), "%d", group->first);
            if (written <= 0) {
                return NETMD_ERROR;
            }
            write_ptr += written;

            if (group->last != group->first && group->last != 0) {
                written = snprintf(write_ptr, (intptr_t)(end_ptr - write_ptr), "-%d", group->last);
                if (written <= 0) {
                    return NETMD_ERROR;
                }
                write_ptr += written;
            }
        }

        written = snprintf(write_ptr, (intptr_t)(end_ptr - write_ptr), ";%s//", group->name);
        if (written <= 0) {
            return NETMD_ERROR;
        }
        write_ptr += written;
    }

    return NETMD_NO_ERROR;
}

netmd_error
netmd_write_disc_header(netmd_dev_handle *devh, const minidisc *md)
{
    // Maximum number of characters in the UTOC + 1
    char title[7*255 + 1];
    memset(title, 0, sizeof(title));

    netmd_error err = netmd_generate_disc_title(md, title, sizeof(title));
    if (err != NETMD_NO_ERROR) {
        return err;
    }

    netmd_set_disc_title(devh, title);

    return NETMD_NO_ERROR;
}

netmd_error
netmd_update_group_range(netmd_dev_handle *dev, minidisc *md, netmd_group_id group, int first, int last)
{
    if (first == 0) {
        // Make group empty, last is ignored (set to zero)
        last = 0;
    }

    if (last < first) {
        return NETMD_TRACK_DOES_NOT_EXIST;
    }

    int n_tracks = netmd_get_track_count(dev);

    if (first < 0 || first > n_tracks || last > n_tracks) {
        return NETMD_TRACK_DOES_NOT_EXIST;
    }

    for (int i=0; i<md->group_count; ++i) {
        if (i != group && first != 0) {
            netmd_group_t *grp = &md->groups[i];

            if (grp->first == 0 || last < grp->first) {
                // No overlap, new group comes before this group
            } else if (grp->last == 0 || first > grp->last) {
                // No overlap, new group comes after this group
            } else {
                return NETMD_GROUP_OVERLAPS_WITH_EXISTING;
            }
        }
    }

    // TODO: Could reorder groups based on first index, and empty ones to the end? (except disc title, ofc)
    md->groups[group].first = first;
    md->groups[group].last = last;

    return netmd_write_disc_header(dev, md);
}
