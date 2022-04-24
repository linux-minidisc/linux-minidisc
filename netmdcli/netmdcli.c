/* netmdcli.c
 *      Copyright (C) 2017 René Rebe
 *      Copyright (C) 2002, 2003 Marc Britten
 *
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <gcrypt.h>
#include <json.h>
#include <unistd.h>

#include "libnetmd.h"
#include "libusbmd.h"
#include "utils.h"

void print_disc_info(netmd_dev_handle* devh, minidisc *md, json_object *json);
void print_current_track_info(netmd_dev_handle* devh);
void print_syntax();
int check_args(int argc, int min_argc, const char *text);
void import_m3u_playlist(netmd_dev_handle* devh, const char *file);

/* Max line length we support in M3U files... should match MD TOC max */
#define M3U_LINE_MAX	128

static void send_raw_message(netmd_dev_handle* devh, char *pszRaw)
{
    unsigned char cmd[255], rsp[255];
    unsigned int data;
    char szBuf[4];
    size_t cmdlen;
    int rsplen;

    /* check raw message length */
    if ((strlen(pszRaw) % 2) != 0) {
        printf("Error: invalid length of raw message!\n");
        return;
    }

    /* convert hex message to bin */
    cmdlen = 0;
    while (*pszRaw != 0) {
        szBuf[0] = *pszRaw++;
        szBuf[1] = *pszRaw++;
        szBuf[2] = '\0';
        if (sscanf(szBuf, "%02X", &data) != 1) {
            printf("Error: invalid character at byte %lu ('%s')\n", (unsigned long)cmdlen, szBuf);
            return;
        }
        cmd[cmdlen++] = data & 0xff;
    }

    /* send it */
    rsplen = netmd_exch_message(devh, cmd, cmdlen, rsp);
    if (rsplen < 0) {
        printf("Error: netmd_exch_message failed with %d\n", rsplen);
        return;
    }
}

struct json_object* json_time(const netmd_time *time)
{
    char buffer[12];
    sprintf(buffer, "%02d:%02d:%02d.%02d", time->hour, time->minute, time->second, time->frame);
    return json_object_new_string(buffer);
}

void print_time(const netmd_time *time)
{
    printf("%02d:%02d:%02d.%02d", time->hour, time->minute, time->second, time->frame);
}

int main(int argc, char* argv[])
{
    netmd_dev_handle* devh;
    minidisc my_minidisc, *md = &my_minidisc;
    netmd_device *device_list, *netmd;
    long unsigned int i = 0;
    long unsigned int j = 0;
    char name[16];
    uint16_t track, playmode;
    int c;
    netmd_time time;
    netmd_error error;
    FILE *f;
    int exit_code = 0;

    /* by default, log only errors */
    netmd_set_log_level(NETMD_LOG_ERROR);

    /* parse options */
    while (1) {
        c = getopt(argc, argv, "tv");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 't':
            netmd_set_log_level(NETMD_LOG_ALL);
            break;
        case 'v':
            netmd_set_log_level(NETMD_LOG_VERBOSE);
            break;
        default:
            fprintf(stderr, "Unknown option '%c'\n", c);
            break;
        }
    }

    /* update argv and argc after parsing options */
    argv = &argv[optind - 1];
    argc -= (optind - 1);

    /* don't require device init to show help */
    if (argc > 1 && strcmp("help", argv[1]) == 0)
    {
        print_syntax();
        return 0;
    }
    else if (argc > 1 && strcmp("usbids", argv[1]) == 0) {
        json_object *json = json_object_new_array();

        const struct minidisc_usb_device_info *cur = minidisc_usb_device_info_first();
        while (cur != NULL) {
            if (cur->device_type == MINIDISC_USB_DEVICE_TYPE_NETMD) {
                json_object *dev = json_object_new_object();
                json_object_object_add(dev, "vendor_id",  json_object_new_int(cur->vendor_id));
                json_object_object_add(dev, "product_id", json_object_new_int(cur->product_id));
                json_object_object_add(dev, "name",       json_object_new_string(cur->name));
                json_object_array_add(json, dev);
            }

            cur = minidisc_usb_device_info_next(cur);
        }

        printf("%s\n", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));
        return 0;
    }

    error = netmd_init(&device_list, NULL);
    if (error != NETMD_NO_ERROR) {
        printf("Error initializing netmd\n%s\n", netmd_strerror(error));
        return 1;
    }

    if (device_list == NULL) {
        puts("Found no NetMD device(s).");
        return 1;
    }

    /* pick first available device */
    netmd = device_list;

    error = netmd_open(netmd, &devh);
    if(error != NETMD_NO_ERROR)
    {
        printf("Error opening netmd\n%s\n", netmd_strerror(error));
        return 1;
    }

    error = netmd_get_devname(devh, name, 16);
    if (error != NETMD_NO_ERROR)
    {
        printf("Could not get device name\n%s\n", netmd_strerror(error));
        return 1;
    }

    netmd_initialize_disc_info(devh, md);

    // Construct JSON object
    json_object *json = json_object_new_object();
    json_object_object_add(json, "device",  json_object_new_string(netmd->model));
    json_object_object_add(json, "title",   json_object_new_string(md->groups[0].name));

    /* parse commands */
    if(argc > 1)
    {
        if(strcmp("rename", argv[1]) == 0)
        {
            if (!check_args(argc, 3, "rename")) return -1;
            i = strtoul(argv[2], NULL, 10);
            netmd_cache_toc(devh);
            netmd_set_title(devh, i & 0xffff, argv[3]);
            netmd_sync_toc(devh);
        }
        else if(strcmp("move", argv[1]) == 0)
        {
            if (!check_args(argc, 3, "move")) return -1;
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            netmd_move_track(devh, i & 0xffff, j & 0xffff);
        }
        else if(strcmp("groupmove", argv[1]) == 0)
        {
            if (!check_args(argc, 3, "groupmove")) return -1;
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            netmd_move_group(devh, md, j & 0xffff, i & 0xffff);
        }
        else if(strcmp("write", argv[1]) == 0)
        {
            // Probably non-functional for most use cases
            if (!check_args(argc, 2, "write")) return -1;
            if(netmd_write_track(devh, argv[2]) < 0)
            {
                fprintf(stderr, "Error writing track %i\n", errno);
            }
        }
        else if(strcmp("newgroup", argv[1]) == 0)
        {
            if (!check_args(argc, 2, "newgroup")) return -1;
            netmd_create_group(devh, md, argv[2]);
        }
        else if(strcmp("settitle", argv[1]) == 0)
        {
            if (!check_args(argc, 2, "settitle")) return -1;
            netmd_cache_toc(devh);
            netmd_set_disc_title(devh, argv[2], strlen(argv[2]));
            netmd_sync_toc(devh);
        }
        else if(strcmp("group", argv[1]) == 0)
        {
            if (!check_args(argc, 3, "group")) return -1;
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            if(!netmd_put_track_in_group(devh, md, i & 0xffff, j & 0xffff))
            {
                printf("Something screwy happened\n");
            }
        }
        else if(strcmp("retitle", argv[1]) == 0)
        {
            if (!check_args(argc, 3, "retitle")) return -1;
            i = strtoul(argv[2], NULL, 10);
            netmd_set_group_title(devh, md, (unsigned int) i, argv[3]);
        }
        else if(strcmp("play", argv[1]) == 0)
        {
            if( argc > 2 ) {
                i = strtoul(argv[2],NULL, 10);
                netmd_set_track( devh, i & 0xffff );
            }
            netmd_play(devh);
        }
        else if(strcmp("stop", argv[1]) == 0)
        {
            netmd_stop(devh);
        }
        else if(strcmp("pause", argv[1]) == 0)
        {
            netmd_pause(devh);
        }
        else if(strcmp("fforward", argv[1]) == 0)
        {
            netmd_fast_forward(devh);
        }
        else if(strcmp("rewind", argv[1]) == 0)
        {
            netmd_rewind(devh);
        }
        else if(strcmp("next", argv[1]) == 0)
        {
            netmd_track_next(devh);
        }
        else if(strcmp("previous", argv[1]) == 0)
        {
            netmd_track_previous(devh);
        }
        else if(strcmp("restart", argv[1]) == 0)
        {
            netmd_track_restart(devh);
        }
        else if(strcmp("settime", argv[1]) == 0)
        {
            if (!check_args(argc, 4, "settime")) return -1;
            track = strtoul(argv[2], (char **) NULL, 10) & 0xffff;
            if (argc > 6)
            {
                time.hour = strtoul(argv[3], (char **) NULL, 10) & 0xffff;
                time.minute = strtoul(argv[4], (char **) NULL, 10) & 0xff;
                time.second = strtoul(argv[5], (char **) NULL, 10) & 0xff;
                time.frame = strtoul(argv[6], (char **) NULL, 10) & 0xff;
            }
            else
            {
                time.hour = 0;
                time.minute = strtoul(argv[3], (char **) NULL, 10) & 0xff;
                time.second = strtoul(argv[4], (char **) NULL, 10) & 0xff;
                if (argc > 5)
                {
                    time.frame = strtoul(argv[5], (char **) NULL, 10) & 0xff;;
                }
                else
                {
                    time.frame = 0;
                }
            }

            netmd_set_time(devh, track, &time);
        }
        else if(strcmp("m3uimport", argv[1]) == 0)
        {
            if (!check_args(argc, 2, "m3uimport")) return -1;
            import_m3u_playlist(devh, argv[2]);
        }
        else if(strcmp("delete", argv[1]) == 0)
        {
            if (!check_args(argc, 2, "delete")) return -1;
            i = strtoul(argv[2], NULL, 10);
            if (argc > 3)
                j = strtoul(argv[3], NULL, 10);
            else
                j = i;

            if (j < i || j >= 0xffff || i >= 0xffff) {
                netmd_log(NETMD_LOG_ERROR, "delete: invalid track number\n");
                exit_code = 1;
            }
            else {
                netmd_cache_toc(devh);
                int track = (int) j;
                for (track; track >= i && track >= 0; track--) {
                    netmd_log(NETMD_LOG_VERBOSE, "delete: removing track %d\n", track);

                    netmd_delete_track(devh, ((uint16_t) track) & 0xffff);
                    netmd_wait_for_sync(devh);
                }
                netmd_sync_toc(devh);
            }
        }
        else if(strcmp("erase", argv[1]) == 0)
        {
          if (!check_args(argc, 2, "erase")) return -1;

          if (strcmp("force", argv[2]) != 0) {
            netmd_log(NETMD_LOG_ERROR, "erase: 'force' must be given as argument to proceed\n");
            exit_code = 1;
          } else {
            netmd_log(NETMD_LOG_VERBOSE, "erase: executing erase\n");
            netmd_erase_disc(devh);
          }
        }
        else if(strcmp("deletegroup", argv[1]) == 0)
        {
            if (!check_args(argc, 2, "deletegroup")) return -1;
            i = strtoul(argv[2], NULL, 10);
            netmd_delete_group(devh, md, i & 0xffff);
        }
        else if(strcmp("status", argv[1]) == 0) {
            print_current_track_info(devh);
        }
        else if (strcmp("raw", argv[1]) == 0) {
            if (!check_args(argc, 2, "raw")) return -1;
            send_raw_message(devh, argv[2]);
        }
        else if (strcmp("setplaymode", argv[1]) == 0) {
            playmode = 0;
            int i;
            for (i = 2; i < argc; i++) {
                if (strcmp(argv[i], "single") == 0) {
                    playmode |= NETMD_PLAYMODE_SINGLE;
                }
                else if (strcmp(argv[i], "repeat") == 0) {
                    playmode |= NETMD_PLAYMODE_REPEAT;
                }
                else if (strcmp(argv[i], "shuffle") == 0) {
                    playmode |= NETMD_PLAYMODE_SHUFFLE;
                }
            }
            printf("%x\n", playmode);
            netmd_set_playmode(devh, playmode);
        }
        else if (strcmp("capacity", argv[1]) == 0) {
            netmd_disc_capacity capacity;
            netmd_get_disc_capacity(devh, &capacity);

            printf("Recorded:  ");
            print_time(&capacity.recorded);
            printf("\nTotal:     ");
            print_time(&capacity.total);
            printf("\nAvailable: ");
            print_time(&capacity.available);
            printf("\n");
        }
        else if (strcmp("recv", argv[1]) == 0) {
            if (!check_args(argc, 3, "recv")) return -1;
            i = strtoul(argv[2], NULL, 10);
            f = fopen(argv[3], "wb");
            netmd_secure_recv_track(devh, i & 0xffff, f);
            fclose(f);
        }
        else if (strcmp("send", argv[1]) == 0) {
            if (!check_args(argc, 2, "send")) return -1;

            const char *filename = argv[2];
            char *title = NULL;
            if (argc > 3)
                title = argv[3];

            exit_code = netmd_send_track(devh, filename, title) == NETMD_NO_ERROR ? 0 : 1;
        } else if (strcmp("leave", argv[1]) == 0) {
          error = netmd_secure_leave_session(devh);
          netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(error));
        }
        else {
            netmd_log(NETMD_LOG_ERROR, "Unknown command '%s'; use 'help' for list of commands\n", argv[1]);
            exit_code = 1;
        }
    }
    else
        print_disc_info(devh, md, json);

    netmd_clean_disc_info(md);
    netmd_close(devh);
    netmd_clean(&device_list);

    return exit_code;
}

void print_current_track_info(netmd_dev_handle* devh)
{
    uint16_t track;
    char buffer[256];
    netmd_time time;

    /* TODO: error checking */
    netmd_get_position(devh, &time);
    netmd_get_track(devh, &track);
    netmd_request_title(devh, track, buffer, 256);

    printf("Current track: %s \n", buffer);
    printf("Current playback position: ");
    print_time(&time);
    printf("\n");

}

void print_disc_info(netmd_dev_handle* devh, minidisc* md, json_object *json)
{
    uint8_t i = 0;
    int size = 1;
    uint8_t g, group = 0, lastgroup = 0;
    unsigned char bitrate_id;
    unsigned char flags;
    unsigned char channel;
    char *name, buffer[256];
    struct netmd_track time;
    struct netmd_pair const *trprot, *bitrate;

    trprot = bitrate = 0;

    netmd_disc_capacity capacity;
    netmd_get_disc_capacity(devh, &capacity);

    json_object_object_add(json, "recordedTime", json_time(&capacity.recorded));
    json_object_object_add(json, "totalTime", json_time(&capacity.total));
    json_object_object_add(json, "availableTime", json_time(&capacity.available));
    json_object* tracks = json_object_new_array();

    for(i = 0; size >= 0; i++)
    {
        size = netmd_request_title(devh, i, buffer, 256);

        if(size < 0)
        {
            break;
        }

        /* Figure out which group this track is in */
        for( group = 0, g = 1; g < md->group_count; g++ )
        {
            if( (md->groups[g].start <= i+1U) && (md->groups[g].finish >= i+1U ))
            {
                group = g;
                break;
            }
        }
        /* Different to the last group? */
        if( group != lastgroup )
        {
            lastgroup = group;
            if( group )			/* Group 0 is 'no group' */
            {
                //printf("Group: %s\n", md->groups[group].name);
            }
        }
        /* Indent tracks which are in a group */
        if( group )
        {
            //printf("  ");
        }

        netmd_request_track_time(devh, i, &time);
        netmd_request_track_flags(devh, i, &flags);
        netmd_request_track_bitrate(devh, i, &bitrate_id, &channel);

        trprot = find_pair(flags, trprot_settings);
        bitrate = find_pair(bitrate_id, bitrates);

        /* Skip 'LP:' prefix... the codec type shows up in the list anyway*/
        if( strncmp( buffer, "LP:", 3 ))
        {
            name = buffer;
        } else {
            name = buffer + 3;
        }

        /*printf("Track %2i: %-6s %6s - %02i:%02i:%02i - %s\n",
               i, trprot->name, bitrate->name, time.minute,
               time.second, time.tenth, name);*/

        // Format track time
        char time_buf[9];
        sprintf(time_buf, "%02i:%02i:%02i", time.minute, time.second, time.tenth);

        // Create JSON track object and add to array
        json_object* track = json_object_new_object();
        json_object_object_add(track, "no",         json_object_new_int(i));
        json_object_object_add(track, "protect",    json_object_new_string(trprot->name));
        json_object_object_add(track, "bitrate",    json_object_new_string(bitrate->name));
        json_object_object_add(track, "time",       json_object_new_string(time_buf));
        json_object_object_add(track, "name",       json_object_new_string(name));
        json_object_array_add(tracks, track);
    }

    json_object_object_add(json, "tracks", tracks);
    printf("%s\n", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));

    // Clean up JSON object
    json_object_put(json);

    exit(0);

    /* XXX - This needs a rethink with the above method */
    /* groups may not have tracks, print the rest. */
    printf("\n--Empty Groups--\n");
    for(group=1; group < md->group_count; group++)
    {
        if(md->groups[group].start == 0 && md->groups[group].finish == 0) {
            printf("Group: %s\n", md->groups[group].name);
        }

    }

    printf("\n\n");
}

void import_m3u_playlist(netmd_dev_handle* devh, const char *file)
{
    FILE *fp;
    char buffer[M3U_LINE_MAX + 1];
    char *s;
    uint8_t track;
    int discard;

    if( file == NULL )
    {
        printf( "No filename specified\n" );
        print_syntax();
        return;
    }

    if( (fp = fopen( file, "r" )) == NULL )
    {
        printf( "Unable to open file %s: %s\n", file, strerror( errno ));
        return;
    }

    if( ! fgets( buffer, M3U_LINE_MAX, fp )) {
        printf( "File Read error\n" );
        return;
    }
    if( strcmp( buffer, "#EXTM3U\n" )) {
        printf( "Invalid M3U playlist\n" );
        return;
    }

    track = 0;
    discard = 0;
    while( fgets( buffer, M3U_LINE_MAX, fp) != NULL ) {
        /* Chomp newlines */
        s = strchr( buffer, '\n' );
        if( s )
            *s = '\0';

        if( buffer[0] == '#' )
        {
            /* comment, ext3inf etc... we only care about ext3inf */
            if( strncmp( buffer, "#EXTINF:", 8 ))
            {
                printf( "Skip: %s\n", buffer );
            }
            else
            {
                s = strchr( buffer, ',' );
                if( !s )
                {
                    printf( "M3U Syntax error! %s\n", buffer );
                }
                else
                {
                    s++;
                    printf( "Title track %d - %s\n", track, s );
                    netmd_set_title(devh, track, s); /* XXX Handle errors */
                    discard = 1;	/* don't fallback to titling by filename */
                }
            }
        }
        else
        {
            /* Filename line */
            if( discard )
            {
                /* printf( "Discard: %s\n", buffer ); */
                discard = 0;
            }
            else
            {
                /* Try and generate a title from the track name */
                s = strrchr( buffer, '.' ); /* Isolate extension */
                if( s )
                    *s = 0;
                s = strrchr( buffer, '/' ); /* Isolate basename */
                if( !s )
                    s = strrchr( buffer, '\\' ); /* Handle DOS paths? */
                if( !s )
                    s = buffer;
                else
                    s++;

                printf( "Title track %d - %s\n", track, s );
                netmd_set_title(devh, track, s); /* XXX Handle errors */
            }
            track++;
        }
    }
}

void print_syntax()
{
    puts("\nNetMD command line tool");
    puts("Usage: netmd [options] command args\n");
    puts("Options:");
    puts("      -v show debug messages");
    puts("      -t enable tracing of USB command and response data\n");
    puts("Commands:");
    puts("rename # <string> - rename track # to <string> track numbers are off by one (ie track 1 is 0)");
    puts("move #1 #2 - make track #1 track #2");
    puts("groupmove #1 #2 - make group #1 start at track #2 !BUGGY!");
    puts("deletegroup #1 - delete a group, but not the tracks in it");
    puts("group #1 #2 - Stick track #1 into group #2");
    puts("retitle #1 <string> - rename group number #1 to <string>");
    puts("play #1 - play track #");
    puts("fforward - start fast forwarding");
    puts("rewind - start rewinding");
    puts("next - starts next track");
    puts("previous - starts previous track");
    puts("restart - restarts current track");
    puts("pause - pause the unit");
    puts("stop - stop the unit");
    puts("delete #1 [#2] - delete track (or tracks in range #1-#2 if #2 given)");
    puts("erase [force] - erase the disc (the argument 'force' must be given to actually do it)");
    puts("m3uimport <file> - import song and disc title from a playlist");
    puts("send <file> [<string>] - send WAV format audio file to the device and set title to <string> (optional)");
    puts("      Supported file formats: 16 bit pcm (stereo or mono) @44100Hz or");
    puts("         Atrac LP2/LP4 data stored in a WAV container.");
    puts("      Title defaults to file name if not specified.");
    puts("raw - send raw command (hex)");
    puts("setplaymode (single, repeat, shuffle) - set play mode");
    puts("newgroup <string> - create a new group named <string>");
    puts("settitle <string> - manually set the complete disc title (with group information)");
    puts("settime <track> [<hour>] <minute> <second> [<frame>] - seeks to the given timestamp");
    puts("      (if three values are given, they are minute, second and frame)");
    puts("capacity - shows current minidisc capacity (used, available)");
    puts("usbids - Output NetMD USB ID list in JSON format");
    puts("help - show this message\n");
}

int check_args(int argc, int min_argc, const char *text)
{
    /* n is the original argc, incl. program name */
    if (argc > min_argc) {
        return 1;
    }

    fprintf(stderr, "Error: %s requires at least %d arguments\n", text, min_argc);
    return 0;
}
