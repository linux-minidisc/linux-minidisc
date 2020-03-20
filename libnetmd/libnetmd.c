/*
 * libnetmd.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
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
#include <glib.h>

#include "libnetmd.h"
#include "utils.h"

/*! list of known codecs (mapped to protocol ID) that can be used in NetMD devices */
/*! Bertrik: the original interpretation of these numbers as codecs appears incorrect.
  These values look like track protection values instead */
struct netmd_pair const trprot_settings[] = 
{
    {0x00, "UnPROT"},
    {0x03, "TrPROT"},
    {0, 0} /* terminating pair */
};

/*! list of known bitrates (mapped to protocol ID) that can be used in NetMD devices */
struct netmd_pair const bitrates[] =
{
    {NETMD_ENCODING_SP, "Stereo"},
    {NETMD_ENCODING_LP2, "LP2"},
    {NETMD_ENCODING_LP4, "LP4"},
    {0, 0} /* terminating pair */
};

struct netmd_pair const unknown_pair = {0x00, "UNKNOWN"};

struct netmd_pair const* find_pair(int hex, struct netmd_pair const* array)
{
    int i = 0;
    for(; array[i].name != NULL; i++)
    {
        if(array[i].hex == hex)
            return &array[i];
    }

    return &unknown_pair;
}

static void waitforsync(libusb_device_handle* dev)
{
    unsigned char syncmsg[4];
    fprintf(stderr,"Waiting for Sync: \n");
    do {
        libusb_control_transfer(dev, 0xc1, 0x01, 0, 0, syncmsg, 0x04, 5000);
    } while  (memcmp(syncmsg,"\0\0\0\0",4)!=0);

}

static unsigned char* sendcommand(netmd_dev_handle* devh, unsigned char* str, const size_t len, unsigned char* response, int rlen)
{
    int i, ret, size = 0;
    static unsigned char buf[256];

    ret = netmd_exch_message(devh, str, len, buf);
    if (ret < 0) {
        fprintf(stderr, "bad ret code, returning early\n");
        return NULL;
    }

    /* Calculate difference to expected response */
    if (response != NULL) {
        int c=0;
        for (i=0; i < min(rlen, size); i++) {
            if (response[i] != buf[i]) {
                c++;
            }
        }
        fprintf(stderr, "Differ: %d\n",c);
    }

    return buf;
}

static int request_disc_title(netmd_dev_handle* dev, char* buffer, size_t size)
{
    int ret = -1;
    size_t title_response_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x01, 0x00, 0x00, 0x30, 0x00, 0xa,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char title[255];
    GError * err = NULL;

    ret = netmd_exch_message(dev, title_request, 0x13, title);
    if(ret < 0)
    {
        fprintf(stderr, "request_disc_title: bad ret code, returning early\n");
        return 0;
    }

    title_response_size = (size_t)ret;

    if(title_response_size == 0 || title_response_size == 0x13)
        return -1; /* bail early somethings wrong */

    int title_response_header_size = 25;
    const char *title_text = title + title_response_header_size;
    size_t encoded_title_size = title_response_size - title_response_header_size;

    char * decoded_title_text;
    decoded_title_text = g_convert(title_text, encoded_title_size, "UTF-8", "SHIFT_JIS", NULL, NULL, &err);

    if(err)
    {
        printf("request_disc_title: title couldn't be converted from SHIFT_JIS to UTF-8: %s", err->message);
        return -1;
    }

    size_t decoded_title_size = strlen(decoded_title_text);

    if(decoded_title_size >= size)
    {
        printf("request_disc_title: title too large for buffer\n");
        return -1;
    }

    memset(buffer, 0, size);
    memcpy(buffer, decoded_title_text, decoded_title_size);

    return title_response_size;
}

int netmd_request_track_time(netmd_dev_handle* dev, const uint16_t track, struct netmd_track* buffer)
{
    int ret = 0;
    int size = 0;
    unsigned char request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x10,
                               0x01, 0x00, 0x01, 0x30, 0x00, 0x01,
                               0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                               0x00};
    unsigned char time_request[255];
    unsigned char *buf;

    buf = request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, request, 0x13, time_request);
    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    size = ret;

    buffer->minute = bcd_to_proper(time_request + 28, 1) & 0xff;
    buffer->second = bcd_to_proper(time_request + 29, 1) & 0xff;
    buffer->tenth = bcd_to_proper(time_request + 30, 1) & 0xff;
    buffer->track = track;

    return 1;
}

int netmd_set_title(netmd_dev_handle* dev, const uint16_t track, const char* const buffer)
{
    int ret = 1;
    unsigned char *title_request = NULL;
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

    ret = netmd_exch_message(dev, title_request, 0x15 + size, reply);
    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    free(title_request);
    return 1;
}

int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x43, 0xff, 0x00, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x04, 0x20,
                               0x10, 0x01, 0x00, 0x03};
    unsigned char reply[255];
    unsigned char *buf;

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, start, 0);

    buf = request + 14;
    netmd_copy_word_to_buffer(&buf, finish, 0);

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

int netmd_set_group_title(netmd_dev_handle* dev, minidisc* md, unsigned int group, char* title)
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
    printf("Raw title: %s \n", disc);

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

int netmd_create_group(netmd_dev_handle* dev, minidisc* md, char* name)
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

int netmd_set_disc_title(netmd_dev_handle* dev, char* title, size_t title_length)
{
    unsigned char *request, *p;
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00};
    unsigned char reply[256];
    int result;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = request_disc_title(dev, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    request = malloc(21 + title_length);
    memset(request, 0, 21 + title_length);

    memcpy(request, write_req, 16);
    request[16] = title_length & 0xff;
    request[20] = oldsize & 0xff;

    p = request + 21;
    memcpy(p, title, title_length);

    result = netmd_exch_message(dev, request, 0x15 + title_length, reply);
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
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00};
    unsigned char reply[255];
    int ret;

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


int netmd_write_track(netmd_dev_handle* devh, char* szFile)
{
    int ret = 0;
    int transferred = 0;
    int fd = open(szFile, O_RDONLY); /* File descriptor to omg file */
    unsigned char *data = malloc(4096); /* Buffer for reading the omg file */
    unsigned char *p = NULL; /* Pointer to index into data */
    uint16_t track_number='\0'; /* Will store the track number of the recorded song */

    /* Some unknown command being send before titling */
    unsigned char begintitle[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02,
                                  0x03, 0x00};

    /* Some unknown command being send after titling */
    unsigned char endrecord[] =  {0x00, 0x18, 0x08, 0x10, 0x18, 0x02,
                                  0x00, 0x00};

    /* Command to finish toc flashing */
    unsigned char fintoc[] = {0x00, 0x18, 0x00, 0x08, 0x00, 0x46,
                              0xf0, 0x03, 0x01, 0x03, 0x48, 0xff,
                              0x00, 0x10, 0x01, 0x00, 0x25, 0x8f,
                              0xbf, 0x09, 0xa2, 0x2f, 0x35, 0xa3,
                              0xdd};

    /* Record command */
    unsigned char movetoendstartrecord[] = {0x00, 0x18, 0x00, 0x08, 0x00, 0x46,
                                            0xf0, 0x03, 0x01, 0x03, 0x28, 0xff,
                                            0x00, 0x01, 0x00, 0x10, 0x01, 0xff,
                                            0xff, 0x00, 0x94, 0x02, 0x00, 0x00,
                                            0x00, 0x06, 0x00, 0x00, 0x04, 0x98};

    /* The expected response from the record command. */
    unsigned char movetoendresp[] = {0x0f, 0x18, 0x00, 0x08, 0x00, 0x46,
                                     0xf0, 0x03, 0x01, 0x03, 0x28, 0x00,
                                     0x00, 0x01, 0x00, 0x10, 0x01, 0x00,
                                     0x11, 0x00, 0x94, 0x02, 0x00, 0x00,
                                     0x43, 0x8c, 0x00, 0x32, 0xbc, 0x50};

    /* Header to be inserted at every 0x3F10 bytes */
    unsigned char header[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x3f, 0x00, 0xd4, 0x4b, 0xdc, 0xaa,
                              0xef, 0x68, 0x22, 0xe2};

    /*	char debug[]  =      {0x4c, 0x63, 0xa0, 0x20, 0x82, 0xae, 0xab, 0xa1}; */
    unsigned char size_request[4];
    size_t data_size_i; /* the size of the data part, later it will be used to point out the last byte in file */
    unsigned int size;
    unsigned char* buf=NULL; /* A buffer for recieving file info */
    libusb_device_handle *dev;

    dev = (libusb_device_handle *)devh;

    if(fd < 0)
        return fd;

    if(!data)
        return ENOMEM;


    /********** Get the size of file ***********/
    lseek(fd, 0x56, SEEK_SET); /* Read to data size */
    read(fd,data,4);
    data_size_i = (size_t)(data[3] + (data[2] << 8) + (data[1] << 16) + (data[0] << 24));

    fprintf(stderr,"Size of data: %lu\n", (unsigned long)data_size_i);
    size = (data_size_i/0x3f18)*8+data_size_i+8;           /* plus number of data headers */
    fprintf(stderr,"Size of data w/ headers: %d\n",size);


    /********** Fill in information in start record command and send ***********/
    /* not sure if size is 3 of 4 bytes in rec command... */
    movetoendstartrecord[27]=(size >> 16) & 255;
    movetoendstartrecord[28]=(size >> 8) & 255;
    movetoendstartrecord[29]=size & 255;

    buf = (unsigned char*)sendcommand(devh, movetoendstartrecord, 30, movetoendresp, 0x1e);
    track_number = buf[0x12] & 0xff;


    /********** Prepare to send data ***********/
    lseek(fd, 90, SEEK_SET);  /* seek to 8 bytes before leading 8 00's */
    data_size_i += 90;        /* data_size_i will now contain position of last byte to be send */

    waitforsync(dev);   /* Wait for 00 00 00 00 from unit.. */


    /********** Send data ***********/
    while(ret >= 0)
    {
        size_t file_pos=0;	/* Position in file */
        size_t bytes_to_send;    /* The number of bytes that needs to be send in this round */

        size_t __bytes_left;     /* How many bytes are left in the file */
        size_t __chunk_size;     /* How many bytes are left in the 0x1000 chunk to send */
        size_t __distance_to_header; /* How far till the next header insert */

        file_pos = (size_t)lseek(fd,0,SEEK_CUR); /* Gets the position in file, might be a nicer way of doing this */

        fprintf(stderr,"pos: %lu/%lu; remain data: %ld\n",
                (unsigned long)file_pos, (unsigned long)data_size_i,
                (signed long)(data_size_i - file_pos));
        if (file_pos >= data_size_i) {
            fprintf(stderr,"Done transferring.\n");
            free(data);
            break;
        }

        __bytes_left = data_size_i - file_pos;
        __chunk_size = 0x1000;
        if (__bytes_left < 0x1000) {
            __chunk_size = __bytes_left;
        }

        __distance_to_header = (file_pos-0x5a) % 0x3f10;
        if (__distance_to_header !=0) __distance_to_header = 0x3f10 - __distance_to_header;
        bytes_to_send = __chunk_size;

        fprintf(stderr,"Chunksize: %lu\n", (unsigned long)__chunk_size);
        fprintf(stderr,"distance_to_header: %lu\n", (unsigned long)__distance_to_header);
        fprintf(stderr,"Bytes left: %lu\n", (unsigned long)__bytes_left);

        if (__distance_to_header <= 0x1000) {   	     /* every 0x3f10 the header should be inserted, with an appropiate key.*/
            fprintf(stderr,"Inserting header\n");

            if (__chunk_size<0x1000) {                 /* if there is room for header then make room for it.. */
                __chunk_size += 0x10;              /* Expand __chunk_size */
                bytes_to_send = __chunk_size-0x08; /* Expand bytes_to_send */
            }

            read(fd,data, __distance_to_header ); /* Errors checking should be added for read function */
            __chunk_size -= __distance_to_header; /* Update chunk size */

            p = data+__distance_to_header;        /* Change p to point at the position header should be inserted */
            memcpy(p,header,16);
            if (__bytes_left-__distance_to_header-0x10 < 0x3f00) {
                __bytes_left -= (__distance_to_header + 0x10);
            }
            else {
                __bytes_left = 0x3f00;
            }

            fprintf (stderr, "bytes left in chunk: %lu\n", (unsigned long)__bytes_left);
            p[6] = (__bytes_left >> 8) & 0xff;      /* Inserts the higher 8 bytes of the length */
            p[7] = __bytes_left & 0xff;     /* Inserts the lower 8 bytes of the length */
            __chunk_size -= 0x10;          /* Update chunk size (for inserted header */

            p += 0x10;                     /* p should now point at the beginning of the next data segment */
            lseek(fd,8,SEEK_CUR);          /* Skip the key value in omg file.. Should probably be used for generating the header */
            read(fd,p, __chunk_size);      /* Error checking should be added for read function */

        } else {
            if(0 == read(fd, data, __chunk_size)) { /* read in next chunk */
                free(data);
                break;
            }
        }

        netmd_log(NETMD_LOG_DEBUG, "Sending %d bytes to md\n", bytes_to_send);
        netmd_log_hex(NETMD_LOG_DEBUG, data, bytes_to_send);
        ret = libusb_bulk_transfer(dev, 0x02, data, (int)bytes_to_send, &transferred, 5000);
    } /* End while */

    if (ret<0) {
        free(data);
        return ret;
    }


    /******** End transfer wait for unit ready ********/
    fprintf(stderr,"Waiting for Done:\n");
    do {
        libusb_control_transfer(dev, 0xc1, 0x01, 0, 0, size_request, 0x04, 5000);
    } while  (memcmp(size_request,"\0\0\0\0",4)==0);

    netmd_log(NETMD_LOG_DEBUG, "Recieving response: \n");
    netmd_log_hex(NETMD_LOG_DEBUG, size_request, 4);
    size = size_request[2];
    if (size<1) {
        fprintf(stderr, "Invalid size\n");
        return -1;
    }
    buf = malloc(size);
    libusb_control_transfer(dev, 0xc1, 0x81, 0, 0, buf, (int)size, 500);
    netmd_log_hex(NETMD_LOG_DEBUG, buf, size);
    free(buf);

    /******** Title the transfered song *******/
    buf = (unsigned char*)sendcommand(devh, begintitle, 8, NULL, 0);

    fprintf(stderr,"Renaming track %d to test\n",track_number);
    netmd_set_title(devh, track_number, "test");

    buf = (unsigned char*)sendcommand(devh, endrecord, 8, NULL, 0);


    /********* End TOC Edit **********/
    ret = libusb_control_transfer(dev, 0x41, 0x80, 0, 0, fintoc, 0x19, 800);

    fprintf(stderr,"Waiting for Done: \n");
    do {
        libusb_control_transfer(dev, 0xc1, 0x01, 0, 0, size_request, 0x04, 5000);
    } while  (memcmp(size_request,"\0\0\0\0",4)==0);

    return ret;
}

int netmd_delete_track(netmd_dev_handle* dev, const uint16_t track)
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

int netmd_cache_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

int netmd_sync_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}
