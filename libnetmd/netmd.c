/* netmd.c
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

#include "libnetmd.h"

void print_disc_info(netmd_dev_handle* devh, minidisc *md);
void print_current_track_info(netmd_dev_handle* devh);
void print_syntax();
void import_m3u_playlist(netmd_dev_handle* devh, const char *file);

/* Max line length we support in M3U files... should match MD TOC max */
#define M3U_LINE_MAX	128


static void print_hex(unsigned char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}


static void handle_secure_cmd(netmd_dev_handle* devh, int cmdid, int track)
{
    unsigned int player_id;
    unsigned char ekb_head[] = {
        0x01, 0xca, 0xbe, 0x07, 0x2c, 0x4d, 0xa7, 0xae,
        0xf3, 0x6c, 0x8d, 0x73, 0xfa, 0x60, 0x2b, 0xd1};
    unsigned char ekb_body[] = {
        0x0f, 0xf4, 0x7d, 0x45, 0x9c, 0x72, 0xda, 0x81,
        0x85, 0x16, 0x9d, 0x73, 0x49, 0x00, 0xff, 0x6c,
        0x6a, 0xb9, 0x61, 0x6b, 0x03, 0x04, 0xf9, 0xce};
    unsigned char rand_in[8], rand_out[8];
    unsigned char hash8[8];
    unsigned char hash32[32];

    switch (cmdid) {
    case 0x11:
        if (netmd_secure_cmd_11(devh, &player_id) > 0) {
            fprintf(stdout, "Player id = %04d\n", player_id);
        }
        break;
    case 0x12:
        netmd_secure_cmd_12(devh, ekb_head, ekb_body);
        break;
    case 0x20:
        memset(rand_in, 0, sizeof(rand_in));
        if (netmd_secure_cmd_20(devh, rand_in, rand_out) > 0) {
            fprintf(stdout, "Random =\n");
            print_hex(rand_out, sizeof(rand_out));
        }
        break;
    case 0x21:
        netmd_secure_cmd_21(devh);
        break;
    case 0x22:
        memset(hash32, 0, sizeof(hash32));
        netmd_secure_cmd_22(devh, hash32);
        break;
    case 0x23:
        if (netmd_secure_cmd_23(devh, track, hash8) > 0) {
            fprintf(stdout, "Hash id of track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;
/*case 0x28: TODO */
    case 0x40:
        if (netmd_secure_cmd_40(devh, track, hash8) > 0) {
            fprintf(stdout, "Signature of deleted track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;
    case 0x48:
        memset(hash8, 0, sizeof(hash8));
        if (netmd_secure_cmd_48(devh, track, hash8) > 0) {
            fprintf(stdout, "Signature of downloaded track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;
    case 0x80:
        netmd_secure_cmd_80(devh);
        break;
    case 0x81:
        netmd_secure_cmd_81(devh);
        break;
    default:
        fprintf(stderr, "unsupported secure command\n");
        break;
    }
}


static void send_raw_message(netmd_dev_handle* devh, char *pszRaw)
{
    char cmd[255], rsp[255];
    unsigned int data;
    char szBuf[4];
    int cmdlen, rsplen;

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
            printf("Error: invalid character at byte %d ('%s')\n", cmdlen, szBuf);
            return;
        }
        cmd[cmdlen++] = data;
    }

    /* send it */
    rsplen = netmd_exch_message(devh, cmd, cmdlen, rsp);
    if (rsplen < 0) {
        printf("Error: netmd_exch_message failed with %d\n", rsplen);
        return;
    }
}


int main(int argc, char* argv[])
{
    netmd_dev_handle* devh;
    minidisc my_minidisc, *md = &my_minidisc;
    netmd_device_t	*device_list, *netmd;
    int i = 0;
    int j = 0;
    char name[16];
    int	cmdid, track, playmode, num_dev, c;

    num_dev = netmd_init(&device_list);
    printf("Found %d NetMD device(s).\n", num_dev);
    if (num_dev != 1) {
        return -1;
    }

    /* pick first available device */
    netmd = device_list;

    devh = netmd_open(netmd);
    if(!devh)
    {
        printf("Error opening netmd\n%s\n", strerror(errno));
        return -1;
    }

    if(!netmd_get_devname(devh, name, 16))
    {
        printf("Could not get device name\n");
        return -1;
    }
    printf("%s\n", name);

    netmd_initialize_disc_info(devh, md);
    printf("Disc Title: %s\n\n", md->groups[0].name);

    /* by default, log only errors */
    netmd_trace_level(NETMD_TRACE_ERROR);

    /* parse options */
    while (1) {
        c = getopt(argc, argv, "t");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 't':
            netmd_trace_level(NETMD_TRACE_INFO);
            break;
        default:
            fprintf(stderr, "Unknown option '%c'\n", c);
            break;
        }
    }

    /* update argv and argc after parsing options */
    argv = &argv[optind - 1];
    argc -= (optind - 1);

    /* parse commands */
    if(argc > 1)
    {
        if(strcmp("rename", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            netmd_set_title(devh, i, argv[3]);
        }
        else if(strcmp("move", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            j = strtol(argv[3], NULL, 10);
            netmd_move_track(devh, i, j);
        }
        else if(strcmp("groupmove", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            j = strtol(argv[3], NULL, 10);
            netmd_move_group(devh, md, j, i);
        }
        else if(strcmp("write", argv[1]) == 0)
        {
            if(netmd_write_track(devh, argv[2]) < 0)
            {
                fprintf(stderr, "Error writing track %i\n", errno);
            }
        }
        else if(strcmp("newgroup", argv[1]) == 0)
        {
            netmd_create_group(devh, argv[2]);
        }
        else if(strcmp("group", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            j = strtol(argv[3], NULL, 10);
            if(!netmd_put_track_in_group(devh, md, i, j))
            {
                printf("Something screwy happened\n");
            }
        }
        else if(strcmp("retitle", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            netmd_set_group_title(devh, md, i, argv[3]);
        }
        else if(strcmp("writeheader", argv[1]) == 0)
        {
            /* edit group cause my logic fucked it up. */
            i = strtol(argv[2], NULL, 10);
            md->groups[i].start = strtol(argv[3], NULL, 10);
            md->groups[i].finish = strtol(argv[4], NULL, 10);
            netmd_write_disc_header(devh, md);
        }
        else if(strcmp("play", argv[1]) == 0)
        {
            if( argc > 2 ) {
                i = strtol(argv[2],NULL, 10);
                netmd_set_track( devh, i );
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
        else if(strcmp("test", argv[1]) == 0)
        {
            test(devh);
        }
        else if(strcmp("m3uimport", argv[1]) == 0)
        {
            import_m3u_playlist(devh, argv[2]);
        }
        else if(strcmp("delete", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            netmd_delete_track(devh, i);
        }
        else if(strcmp("deletegroup", argv[1]) == 0)
        {
            i = strtol(argv[2], NULL, 10);
            netmd_delete_group(devh, md, i);
        }
        else if(strcmp("status", argv[1]) == 0) {
            print_current_track_info(devh);
        }
        else if (strcmp("raw", argv[1]) == 0) {
            send_raw_message(devh, argv[2]);
        }
        else if (strcmp("setplaymode", argv[1]) == 0) {
            playmode = 0;
            for (i = 2; i < argc; i++) {
                if (strcmp(argv[i], "single") == 0) {
                    playmode |= NETMD_PLAYMODE_SINGLE;
                }
                if (strcmp(argv[i], "repeat") == 0) {
                    playmode |= NETMD_PLAYMODE_REPEAT;
                }
                if (strcmp(argv[i], "shuffle") == 0) {
                    playmode |= NETMD_PLAYMODE_SHUFFLE;
                }
            }
            netmd_set_playmode(devh, playmode);
        }
        else if (strcmp("secure", argv[1]) == 0) {
            cmdid = strtol(argv[2], NULL, 16);
            track = 0;
            if (argc > 3) {
                track = strtol(argv[3], NULL, 10);
            }
            handle_secure_cmd(devh, cmdid, track);
        }
        else if(strcmp("help", argv[1]) == 0)
        {
            print_syntax();
        }
        else
        {
            print_disc_info(devh, md);
            print_syntax();
        }
    }
    else
        print_disc_info(devh, md);

    netmd_clean_disc_info(md);
    netmd_close(devh);
    netmd_clean(device_list);

    return 0;
}

void print_current_track_info(netmd_dev_handle* devh) {
    float f = 0;
    int i = 0;
    int size = 0;
    char buffer[256];


    f = netmd_get_playback_position(devh);
    i = netmd_get_current_track(devh);

    size = netmd_request_title(devh, i, buffer, 256);

    if(size < 0)
    {
        printf("something really really nasty just happened.");
    }
    else {
        printf("Current track: %s \n", buffer);
    }

    printf("Current playback position: %f \n", f);

}

void print_disc_info(netmd_dev_handle* devh, minidisc* md)
{
    int i = 0;
    int size = 1;
    int g, group = 0, lastgroup = 0;
    unsigned char bitrate_id;
    char flags;
    char *name, buffer[256];
    struct netmd_track time;
    struct netmd_pair const *trprot, *bitrate;

    trprot = bitrate = 0;

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
            if( (md->groups[g].start <= i+1) && (md->groups[g].finish >= i+1 ))
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
                printf("Group: %s\n", md->groups[group].name);
            }
        }
        /* Indent tracks which are in a group */
        if( group )
        {
            printf("  ");
        }

        netmd_request_track_time(devh, i, &time);
        netmd_request_track_flags(devh, i, &flags);
        netmd_request_track_bitrate(devh, i, &bitrate_id);

        trprot = find_pair(flags, trprot_settings);
        bitrate = find_pair(bitrate_id, bitrates);

        /* Skip 'LP:' prefix... the codec type shows up in the list anyway*/
        if( strncmp( buffer, "LP:", 3 ))
        {
            name = buffer;
        }
        else
        {
            name = buffer + 3;
        }

        printf("Track %2i: %-6s %6s - %02i:%02i:%02i - %s\n",
               i, trprot->name, bitrate->name, time.minute,
               time.second, time.tenth, name);
    }

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
    int track, discard;

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
    puts("\nNetMD test suite.\nUsage: netmd [options] command args\n");
    puts("Options:\n  -t enable tracing of USB command and response data\n");
    puts("Commands:\n");
    puts("rename # <string> - rename track # to <string> track numbers are off by one (ie track 1 is 0)\n");
    puts("move #1 #2 - make track #1 track #2\n");
    puts("groupmove #1 #2 - make group #1 start at track #2 !BUGGY!\n");
    puts("deletegroup #1 - delete a group, but not the tracks in it\n");
    puts("write <string> - write omg file to netmd unit !DOES NOT WORK!\n");
    puts("group #1 #2 - Stick track #1 into group #2\n");
    puts("retitle #1 <string> - rename group number #1 to <string>\n");
    puts("play #1 - play track #\n");
    puts("fforward - start fast forwarding\n");
    puts("rewind - start rewinding\n");
    puts("pause - pause the unit\n");
    puts("stop - stop the unit\n");
    puts("delete #1 - delete track\n");
    puts("m3uimport - import playlist - and title current disc using it.\n");
    puts("raw - send raw command (hex)\n");
    puts("setplaymode (single, repeat, shuffle) - set play mode\n");
    puts("newgroup <string> - create a new group named <string>\n");
    puts("secure #1 #2 - execute secure command #1 on track #2 (where applicable)\n");
    puts("  --- general ---\n");
    puts("  0x80 = start secure session\n");
    puts("  0x11 = get player id\n");
    puts("  0x12 = send ekb\n");
    puts("  0x20 = exchange randoms\n");
    puts("  0x21 = discard randoms\n");
    puts("  0x81 = end secure session\n");
    puts("  --- check-out ---\n");
    puts("  0x22 = submit 32-byte hash\n");
    puts("  0x28 = prepare download\n");
    puts("  0x48 = verify downloaded track #\n");
    puts("  --- check-in ---\n");
    puts("  0x23 = get hash id for track #\n");
    puts("  0x40 = secure delete track #\n");
    puts("help - print this stuff\n");
}
