/*
 * libnetmd.c
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

#include <unistd.h>

#include <libusb.h>

#include "libnetmd.h"
#include "utils.h"

const char *
netmd_get_encoding_name(enum NetMDEncoding encoding, enum NetMDChannels channels)
{
    switch (channels) {
        case NETMD_CHANNELS_MONO:
            return "Mono";
        case NETMD_CHANNELS_STEREO:
            switch (encoding) {
                case NETMD_ENCODING_SP:
                    return "SP";
                case NETMD_ENCODING_LP2:
                    return "LP2";
                case NETMD_ENCODING_LP4:
                    return "LP4";
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return "UNKNOWN";
}

const char *
netmd_track_flags_to_string(enum NetMDTrackFlags flags)
{
    switch (flags) {
        case NETMD_TRACK_FLAG_UNPROTECTED:
            return "UnPROT";
        case NETMD_TRACK_FLAG_PROTECTED:
            return "TrPROT";
        default:
            return "UNKNOWN";
    }
}

static int request_disc_title(netmd_dev_handle* dev, char* buffer, size_t size)
{
    int ret = -1;
    size_t title_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x01, 0x00, 0x00, 0x30, 0x00, 0xa,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char title[255];

    ret = netmd_exch_message(dev, title_request, 0x13, title);
    if(ret < 0)
    {
        fprintf(stderr, "request_disc_title: bad ret code, returning early\n");
        return 0;
    }

    title_size = (size_t)ret;

    if(title_size == 0 || title_size == 0x13)
        return -1; /* bail early somethings wrong */

    if((title_size - 25) >= size)
    {
        printf("request_disc_title: title too large for buffer\n");
    }
    else
    {
        memset(buffer, 0, size);
        memcpy(buffer, (title + 25), title_size - 25);
        buffer[title_size - 25] = 0;
    }

    return (int)title_size - 25;
}

int netmd_set_track_title(netmd_dev_handle* dev, netmd_track_index track, const char *buffer)
{
    int ret = 1;
    unsigned char *title_request = NULL;
    /* handshakes for 780/980/etc */
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};

    unsigned char title_header[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                    0x02, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                    0x00, 0x50, 0x00, 0x00, 0x0a, 0x00,
                                    0x00, 0x00, 0x0d};
    unsigned char reply[255];
    unsigned char *buf;
    size_t size;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = netmd_request_title(dev, track, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    size = strlen(buffer);
    title_request = malloc(sizeof(char) * (0x15 + size));
    memcpy(title_request, title_header, 0x15);
    memcpy((title_request + 0x15), buffer, size);

    buf = title_request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    title_request[16] = size & 0xff;
    title_request[20] = oldsize & 0xff;


    /* send handshakes */
    netmd_exch_message(dev, hs2, 8, reply);
    netmd_exch_message(dev, hs3, 8, reply);

    ret = netmd_exch_message(dev, title_request, 0x15 + size, reply);
    free(title_request);

    if(ret < 0)
    {
        netmd_log(NETMD_LOG_WARNING, "netmd_set_title: exchange failed, ret=%d\n", ret);
        return 0;
    }

    return 1;
}

int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x00, 0x00};
    unsigned char request[] = {0x00, 0x18, 0x43, 0xff, 0x00, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x04, 0x20,
                               0x10, 0x01, 0x00, 0x03};
    unsigned char reply[255];
    unsigned char *buf;

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, start, 0);

    buf = request + 14;
    netmd_copy_word_to_buffer(&buf, finish, 0);

    netmd_exch_message(dev, hs, 8, reply);
    netmd_exch_message(dev, request, 16, reply);
    ret = netmd_exch_message(dev, request, 16, reply);

    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    return 1;
}

static unsigned int get_group_count(netmd_dev_handle* devh)
{
    char title[256];
    int title_length;
    char *group;
    char *delim;
    unsigned int group_count = 1;

    title_length = request_disc_title(devh, title, 256);

    group = title;
    delim = strstr(group, "//");

    while (delim < (title + title_length))
    {
        if (delim != NULL)
        {
            /* if delimiter was found */
            delim[0] = '\0';
        }

        if (strlen(group) > 0) {
            if (atoi(group) > 0 || group[0] == ';') {
                group_count++;
            }
        }

        if (NULL == delim)
        {
            /* finish if delimiter was not found the last time */
            break;
        }

        if (delim+2 > title+title_length)
        {
            /* finish if delimiter was at end of title */
            break;
        }

        group = delim + 2;
        delim = strstr(group, "//");
    }

    return group_count;
}

int netmd_set_group_title(netmd_dev_handle* dev, minidisc* md, unsigned int group, const char* title)
{
    size_t size = strlen(title);

    md->groups[group].name = realloc(md->groups[group].name, size + 1);

    if(md->groups[group].name != 0)
        strcpy(md->groups[group].name, title);
    else
        return 0;

    netmd_write_disc_header(dev, md);

    return 1;
}

static void set_group_data(minidisc* md, const int group, const char* const name, const uint16_t start, const uint16_t finish) {
    md->groups[group].name = strdup(name);
    md->groups[group].start = start;
    md->groups[group].finish = finish;
    return;
}

/* Sonys code is utter bile. So far we've encountered the following first segments in the disc title:
 *
 * 0[-n];<title>// - titled disc.
 * <title>// - titled disc
 * 0[-n];// - untitled disc
 * n{n>0};<group>// - untitled disc, group with one track
 * n{n>0}-n2{n2>n>0};group// - untitled disc, group with multiple tracks
 * ;group// - untitled disc, group with no tracks
 *
 */

int netmd_initialize_disc_info(netmd_dev_handle* devh, minidisc* md)
{
    int disc_size = 0;
    char disc[256];

    md->group_count = get_group_count(devh);

    /* You always have at least one group, the disc title */
    if(md->group_count == 0)
        md->group_count++;

    md->groups = malloc(sizeof(struct netmd_group) * md->group_count);
    memset(md->groups, 0, sizeof(struct netmd_group) * md->group_count);

    disc_size = request_disc_title(devh, disc, 256);

    if(disc_size > 0)
    {
        md->header_length = (size_t)disc_size;
        netmd_parse_disc_title(md, disc, md->header_length);
    }

    if (NULL == md->groups[0].name)
    {
        /* set untitled disc title */
        set_group_data(md, 0, "<Untitled>", 0, 0);
    }

    return disc_size;
}

void netmd_parse_disc_title(minidisc* md, char* title, size_t title_length)
{
    char *group;
    char *delim;
    int group_count = 1;

    group = title;
    delim = strstr(group, "//");

    while (delim < (title + title_length))
    {
        if (delim != NULL)
        {
            /* if delimiter was found */
            delim[0] = '\0';
        }

        netmd_parse_group(md, group, &group_count);

        if (NULL == delim)
        {
            /* finish if delimiter was not found the last time */
            break;
        }

        group = delim + 2;

        if (group > (title + title_length))
        {
            /* end of title */
            break;
        }

        delim = strstr(group, "//");
    }
}

void netmd_parse_group(minidisc* md, char* group, int* group_count)
{
    char *group_name;

    group_name = strchr(group, ';');
    if (NULL == group_name)
    {
        if (strlen(group) > 0)
        {
            /* disc title */
            set_group_data(md, 0, group, 0, 0);
        }
    }
    else
    {
        group_name[0] = '\0';
        group_name++;

        if (strlen(group_name) > 0)
        {
            if (0 == strlen(group))
            {
                set_group_data(md, *group_count, group_name, 0, 0);
                (*group_count)++;
            }
            else
            {
                netmd_parse_trackinformation(md, group_name, group_count, group);
            }
        }
    }
}

void netmd_parse_trackinformation(minidisc* md, char* group_name, int* group_count, char* tracks)
{
    char *track_last;
    uint16_t start, finish;

    start = strtoul(tracks, (char **) NULL, 10) & 0xffffU;
    if (start == 0)
    {
        /* disc title */
        set_group_data(md, 0, group_name, 0, 0);
    }
    else {
        track_last = strchr(tracks, '-');

        if (NULL == track_last)
        {
            finish = start;
        }
        else
        {
            track_last[0] = '\0';
            track_last++;

            finish = strtoul(track_last, (char **) NULL, 10) & 0xffffU;
        }

        set_group_data(md, *group_count, group_name, start, finish);
        (*group_count)++;
    }
}

void print_groups(minidisc *md)
{
    unsigned int i;
    for(i = 0; i < md->group_count; i++)
    {
        printf("Group %i: %s - %i - %i\n", i, md->groups[i].name, md->groups[i].start, md->groups[i].finish);
    }
    printf("\n");
}

int netmd_create_group(netmd_dev_handle* dev, minidisc* md, const char* name)
{
    unsigned int new_index;

    new_index = md->group_count;
    md->group_count++;
    md->groups = realloc(md->groups, sizeof(struct netmd_group) * (md->group_count + 1));

    md->groups[new_index].name = strdup(name);
    md->groups[new_index].start = 0;
    md->groups[new_index].finish = 0;

    netmd_write_disc_header(dev, md);
    return 0;
}

int netmd_set_disc_title(netmd_dev_handle* dev, const char* title)
{
    unsigned char *request, *p;
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00};
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};
    unsigned char hs4[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char reply[256];
    int result;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = request_disc_title(dev, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    size_t title_length = strlen(title);

    request = malloc(21 + title_length);
    memset(request, 0, 21 + title_length);

    memcpy(request, write_req, 16);
    request[16] = title_length & 0xff;
    request[20] = oldsize & 0xff;

    p = request + 21;
    memcpy(p, title, title_length);

    /* send handshakes */
    netmd_exch_message(dev, hs1, sizeof(hs1), reply);
    netmd_exch_message(dev, hs2, sizeof(hs2), reply);
    netmd_exch_message(dev, hs3, sizeof(hs3), reply);
    result = netmd_exch_message(dev, request, 0x15 + title_length, reply);
    /* send handshake to write */
    netmd_exch_message(dev, hs4, sizeof(hs4), reply);
    return result;
}

/* move track, then manipulate title string */
int netmd_put_track_in_group(netmd_dev_handle* dev, minidisc *md, const uint16_t track, const unsigned int group)
{
    unsigned int i = 0;
    unsigned int j = 0;
    int found = 0;

    printf("netmd_put_track_in_group(dev, %i, %i)\nGroup Count %i\n", track, group, md->group_count);

    if (group >= md->group_count)
    {
        return 0;
    }

    print_groups(md);

    /* remove track from old group */
    for(i = 0; i < md->group_count; i++)
    {
        if(i == 0)
        {
            /* if track is before first real group */
            if(md->groups[i+1].start == 0)
            {
                /* nothing in group  */
                found = 1;
            }
            if(((track + 1U) & 0xffffU) < md->groups[i+1].start)
            {
                found = 1;
                for(j = i+1; j < md->group_count; j++)
                {
                    md->groups[j].start--;
                    if(md->groups[j].finish != 0)
                        md->groups[j].finish--;
                }
            }
        }
        else if(md->groups[i].start < track && md->groups[i].finish > track)
        {
            found = 1;
            /* decrement start/finish for all following groups */
            for(j = i+1; j < md->group_count; j++)
            {
                md->groups[j].start--;
                if(md->groups[j].finish != 0)
                    md->groups[j].finish--;
            }
        }
    }

    /* if track is in between groups */
    if(!found)
    {
        for(i = 2; i < md->group_count; i++)
        {
            if(md->groups[i].start > track && md->groups[i-1].finish < track)
            {
                found = 1;
                /* decrement start/finish for all groups including and after this one */
                for(j = i; j < md->group_count; j++)
                {
                    md->groups[j].start--;
                    if(md->groups[j].finish != 0)
                        md->groups[j].finish--;
                }
            }
        }
    }

    print_groups(md);

    /* insert track into group range */
    if(md->groups[group].finish != 0)
    {
        md->groups[group].finish++;
    }
    else
    {
        if(md->groups[group].start == 0)
            md->groups[group].start = (track + 1U) & 0xffffU;
        else
            md->groups[group].finish = (md->groups[group].start + 1U) & 0xffffU;
    }

    /* if not last group */
    if((group + 1) < md->group_count)
    {
        unsigned int j = 0;
        for(j = group + 1; j < md->group_count; j++)
        {
            /* if group is NOT empty */
            if(md->groups[j].start != 0 || md->groups[j].finish != 0)
            {
                md->groups[j].start++;
                if(md->groups[j].finish != 0)
                {
                    md->groups[j].finish++;
                }
            }
        }
    }

    /* what does it look like now? */
    print_groups(md);

    if(md->groups[group].finish != 0)
    {
        netmd_move_track(dev, track, (md->groups[group].finish - 1U) & 0xffffU);
    }
    else
    {
        if(md->groups[group].start != 0)
            netmd_move_track(dev, track, (md->groups[group].start - 1U) & 0xffffU);
        else
            netmd_move_track(dev, track, md->groups[group].start & 0xffffU);
    }

    return netmd_write_disc_header(dev, md);
}

int netmd_move_group(netmd_dev_handle* dev, minidisc* md, const uint16_t track, const unsigned int group)
{
    uint16_t index = 0;
    unsigned int i = 0;
    uint16_t gs = 0;
    struct netmd_group store1;
    struct netmd_group *p, *p2;
    uint16_t gt = md->groups[group].start;
    uint16_t finish = (((unsigned int)md->groups[group].finish - md->groups[group].start) + track) & 0xffffU;

    p = p2 = 0;

    /* empty groups can't be in front */
    if(gt == 0)
        return -1;

    /* loop, moving tracks to new positions */
    for(index = track; index <= finish; index++, gt++)
    {
        printf("Moving track %i to %i\n", (gt - 1U) & 0xffffU, index & 0xffffU);
        netmd_move_track(dev, (gt - 1U) & 0xffffU, index & 0xffffU);
    }
    md->groups[group].start = (track + 1U) & 0xffffU;
    md->groups[group].finish = index;

    /* create a copy of groups */
    p = malloc(sizeof(struct netmd_group) * md->group_count);
    for(index = 0; index < md->group_count; index++)
    {
        p[index].name = malloc(strlen(md->groups[index].name) + 1);
        strcpy(p[index].name, md->groups[index].name);
        p[index].start = md->groups[index].start;
        p[index].finish = md->groups[index].finish;
    }

    store1 = p[group];
    gs = ((unsigned int)store1.finish - store1.start + 1) & 0xffffU; /* how many tracks got moved? */

    /* find group to bump */
    if(track < md->groups[group].start)
    {
        for(index = 0; index < md->group_count; index++)
        {
            if(md->groups[index].start > track)
            {
                for(i = group - 1; i >= index; i--)
                {
                    /* all tracks get moved gs spots */
                    p[i].start = ((unsigned int)p[i].start + gs) & 0xffffU;

                    if(p[i].finish != 0)
                        p[i].finish = ((unsigned int)p[1].finish + gs) & 0xffffU;

                    p[i + 1] = p[i]; /* bump group down the list */
                }

                p[index] = store1;
                break;
            }
            else
            {
                if((group + 1) < md->group_count)
                {
                    for(i = group + 1; i < md->group_count; i++)
                    {
                        /* all tracks get moved gs spots */
                        p[i].start = ((unsigned int)p[i].start - gs) & 0xffffU;;

                        if(p[i].finish != 0)
                            p[i].finish = ((unsigned int)p[1].finish - gs) & 0xffffU;

                        p[i - 1] = p[i]; /* bump group down the list */
                    }

                    p[index] = store1;
                    break;
                }
            }
        }
    }

    /* free all memory, then make our copy the real info */
    netmd_clean_disc_info(md);
    md->groups = p;

    netmd_write_disc_header(dev, md);
    return 0;
}

int netmd_delete_group(netmd_dev_handle* dev, minidisc* md, const unsigned int group)
{
    unsigned int index = 0;
    struct netmd_group *p;

    /* check if requested group exists */
    if(group > md->group_count)
        return -1;

    /* create a copy of groups below requested group */
    p = malloc(sizeof(struct netmd_group) * (md->group_count - 1));
    for(index = 0; index < group; index++)
    {
        p[index].name = md->groups[index].name;
        p[index].start = md->groups[index].start;
        p[index].finish = md->groups[index].finish;
    }

    /* copy groups above requested group */
    for(; index < (md->group_count - 1); index++)
    {
        p[index].name = md->groups[index+1].name;
        p[index].start = md->groups[index+1].start;
        p[index].finish = md->groups[index+1].finish;
    }

    /* free memory, then make our copy the real info */
    free(md->groups);
    md->groups = p;

    /* one less group now */
    md->group_count--;

    netmd_write_disc_header(dev, md);
    return 0;
}

size_t netmd_calculate_number_length(const unsigned int num)
{
    if (num >= 100) {
        return 3;
    }

    if (num >= 10) {
        return 2;
    }

    return 1;

}

size_t netmd_calculate_disc_header_length(minidisc* md)
{
    size_t header_size;
    unsigned int i;

    header_size = 0;
    if (md->groups[0].start == 0)
    {
        /* '\0' for disc title */
        header_size++;
    }

    for(i = 0; i < md->group_count; i++)
    {
        if(md->groups[i].start > 0)
        {
            header_size += netmd_calculate_number_length(md->groups[i].start);

            if(md->groups[i].finish != 0)
            {
                /* '-' */
                header_size++;
                header_size += netmd_calculate_number_length(md->groups[i].finish);
            }
        }

        /* ';' '//' */
        header_size += 3;
        header_size += strlen(md->groups[i].name);
    }

    /* '\0' */
    header_size++;
    return header_size;
}

size_t netmd_calculate_remaining(char** position, size_t remaining, size_t written)
{
    if (remaining > written)
    {
        (*position) += written;
        remaining -= written;
    }
    else
    {
        (*position) += remaining;
        remaining = 0;
    }

    return remaining;
}

char* netmd_generate_disc_header(minidisc* md, char* header, size_t header_length)
{
    unsigned int i;
    size_t remaining, written;
    char* position;
    int result;

    position = header;
    remaining = header_length - 1;

    if (md->groups[0].start == 0)
    {
        strncat(position, "0", remaining);
        written = strlen(position);
        remaining = netmd_calculate_remaining(&position, remaining, written);
    }

    for(i = 0; i < md->group_count; i++)
    {
        if(md->groups[i].start > 0)
        {
            result = snprintf(position, remaining, "%d", md->groups[i].start);
            if (result > 0) {
                written = (size_t)result;
                remaining = netmd_calculate_remaining(&position, remaining, written);

                if(md->groups[i].finish != 0)
                {
                    result = snprintf(position, remaining, "-%d", md->groups[i].finish);
                    if (result > 0) {
                        written = (size_t)result;
                        remaining = netmd_calculate_remaining(&position, remaining, written);
                    }
                }
            }
        }

        result = snprintf(position, remaining, ";%s//", md->groups[i].name);
        if (result > 0) {
            written = (size_t)result;
            remaining = netmd_calculate_remaining(&position, remaining, written);
        }
    }

    position[0] = '\0';
    return header;
}

int netmd_write_disc_header(netmd_dev_handle* devh, minidisc* md)
{

    size_t header_size;
    size_t request_size;
    char* header = 0;
    unsigned char* request = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00};
    unsigned char reply[255];
    int ret;
    printf("sending write disc header handshake");
    netmd_exch_message(devh, hs, 8, reply);
    printf("...OK\n");
    header_size = netmd_calculate_disc_header_length(md);
    header = malloc(sizeof(char) * header_size);
    memset(header, 0, header_size);

    netmd_generate_disc_header(md, header, header_size);

    request_size = header_size + sizeof(write_req);
    request = malloc(request_size);
    memset(request, 0, request_size);

    memcpy(request, write_req, sizeof(write_req));
    request[16] = (header_size - 1) & 0xff; /* new size - null */
    request[20] = md->header_length & 0xff; /* old size */

    memcpy(request + sizeof(write_req), header, header_size);
    header[header_size - 1] = '\0';

    ret = netmd_exch_message(devh, request, request_size, reply);
    free(request);

    return ret;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "specific_object" (0x01) */
int netmd_delete_track(netmd_dev_handle* dev, netmd_track_index track)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x01, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x00};
    unsigned char reply[255];
    unsigned char *buf;

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, request, 11, reply);

    return ret;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "complete" (0x00) */
int netmd_erase_disc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, 11, reply);

    return ret;
}

void netmd_clean_disc_info(minidisc *md)
{
    unsigned int i;
    for(i = 0; i < md->group_count; i++)
    {
        free(md->groups[i].name);
        md->groups[i].name = NULL;
    }

    free(md->groups);
    md->groups = NULL;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "open for write" (0x03) */
int netmd_cache_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "close" (0x00) */
int netmd_sync_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* Calls need for Sharp devices */
int netmd_acquire_dev(netmd_dev_handle* dev)
{
    // Unit command: "subunit_type = 0x1F, subunit_id = 7 -> 0xFF"
    // AVC Digital Interface Commands 3.0, 9. Unit commands"
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    // TODO: Check return value
    netmd_exch_message(dev, request, sizeof(request), reply);
    if (reply[0] == NETMD_STATUS_ACCEPTED){
      return NETMD_NO_ERROR;
    } else {
      return NETMD_COMMAND_FAILED_UNKNOWN_ERROR;
    }
}

int netmd_release_dev(netmd_dev_handle* dev)
{
    // Unit command: "subunit_type = 0x1F, subunit_id = 7 -> 0xFF"
    // AVC Digital Interface Commands 3.0, 9. Unit commands"
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    return netmd_exch_message(dev, request, sizeof(request), reply);
}

netmd_error netmd_get_track_info(netmd_dev_handle *dev, netmd_track_index track_id, struct netmd_track_info *info)
{
    unsigned char flags, bitrate_id, channel;

    size_t title_len = netmd_request_title(dev, track_id, info->raw_title, sizeof(info->raw_title));

    if (title_len == -1) {
        return NETMD_TRACK_DOES_NOT_EXIST;
    }

    info->title = info->raw_title;

    // TODO: All these calls need proper error handling
    netmd_request_track_time(dev, track_id, &info->duration);
    netmd_request_track_flags(dev, track_id, &flags);
    netmd_request_track_bitrate(dev, track_id, &bitrate_id, &channel);

    info->encoding = (enum NetMDEncoding)bitrate_id;
    info->channels = (enum NetMDChannels)channel;
    info->protection = (enum NetMDTrackFlags)flags;

    /*
     * Skip 'LP:' prefix, but only on tracks that are actually LP-encoded,
     * since a SP track might be titled beginning with "LP:" on purpose.
     * Note that since the MZ-R909 the "LP Stamp" can be disabled, so we
     * must check for the existence of the "LP:" prefix before skipping.
     */
    if ((info->encoding == NETMD_ENCODING_LP2 || info->encoding == NETMD_ENCODING_LP4) && strncmp(info->title, "LP:", 3) == 0) {
        info->title += 3;
    }

    return NETMD_NO_ERROR;
}

const char *
netmd_minidisc_get_disc_name(const minidisc *md)
{
    return md->groups[0].name;
}

netmd_group_id
netmd_minidisc_get_track_group(const minidisc *md, netmd_track_index track_id)
{
    // Tracks in the groups are 1-based
    track_id += 1;

    for (int group = 1; group < md->group_count; group++) {
        if ((md->groups[group].start <= track_id) && (md->groups[group].finish >= track_id)) {
            return group;
        }
    }

    return 0;
}

const char *
netmd_minidisc_get_group_name(const minidisc *md, netmd_group_id group)
{
    if (group == 0 || group >= md->group_count) {
        return NULL;
    }

    return md->groups[group].name;
}

bool
netmd_minidisc_is_group_empty(const minidisc *md, netmd_group_id group)
{
    // Non-existent groups are always considered "empty"
    if (group == 0 || group >= md->group_count) {
        return true;
    }

    return (md->groups[group].start == 0 && md->groups[group].finish == 0);
}
