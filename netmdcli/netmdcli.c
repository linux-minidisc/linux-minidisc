/* netmdcli.c
 *      Copyright (C) 2022 Thomas Perl <m@thp.io>
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

#include <json.h>
#include <unistd.h>
#include <getopt.h>

#include "libnetmd.h"
#include "libusbmd.h"
#include "utils.h"

#include "cmds.h"


void usage();

void print_disc_info(netmd_dev_handle* devh, minidisc *md, json_object *json);
void import_m3u_playlist(netmd_dev_handle* devh, const char *file);

/* Max line length we support in M3U files... should match MD TOC max */
#define M3U_LINE_MAX	128

/* Color output */
static const char *
ansi_grp_begin = "\033[37m";

static const char *
ansi_trk_begin = "\033[36m";

static const char *
ansi_sp_begin = "\033[92m";

static const char *
ansi_mono_begin = "\033[91m";

static const char *
ansi_lp2_begin = "\033[94m";

static const char *
ansi_lp4_begin = "\033[95m";

static const char *
ansi_no_title_begin = "\033[90m";

static const char *
ansi_header_begin = "\033[90m";

static const char *
ansi_end = "\033[0m";


static void send_raw_message(netmd_dev_handle* devh, const char *pszRaw)
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
    char *buffer = netmd_time_to_string(time);
    json_object *result = json_object_new_string(buffer);
    netmd_free_string(buffer);
    return result;
}

static int
cmd_rename(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_int_arg(ctx, "track_id") & 0xffff;
    const char *new_title = netmdcli_context_get_string_arg(ctx, "new_title");

    netmd_cache_toc(ctx->devh);
    netmd_set_title(ctx->devh, track_id, new_title);
    netmd_sync_toc(ctx->devh);

    return 0;
}

static int
cmd_play(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_optional_int_arg(ctx, "track_id");

    if (track_id != -1) {
        track_id &= 0xffff;
        netmd_set_current_track(ctx->devh, track_id);
    }

    netmd_play(ctx->devh);

    return 0;
}

static int
cmd_move(struct netmdcli_context *ctx)
{
    int from_track_id = netmdcli_context_get_int_arg(ctx, "from_track_id");
    int to_track_id = netmdcli_context_get_int_arg(ctx, "to_track_id");

    netmd_move_track(ctx->devh, from_track_id & 0xffff, to_track_id & 0xffff);

    return 0;
}

static int
cmd_groupmove(struct netmdcli_context *ctx)
{
    int group_id = netmdcli_context_get_int_arg(ctx, "group_id");
    int start_track_id = netmdcli_context_get_int_arg(ctx, "start_track_id");

    netmd_move_group(ctx->devh, ctx->md, group_id & 0xffff, start_track_id & 0xffff);

    return 0;
}

static int
cmd_deletegroup(struct netmdcli_context *ctx)
{
    int group_id = netmdcli_context_get_int_arg(ctx, "group_id");

    netmd_delete_group(ctx->devh, ctx->md, group_id & 0xffff);

    return 0;
}

static int
cmd_group(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_int_arg(ctx, "track_id");
    int group_id = netmdcli_context_get_int_arg(ctx, "group_id");

    if (!netmd_put_track_in_group(ctx->devh, ctx->md, track_id & 0xffff, group_id & 0xffff)) {
        return 1;
    }

    return 0;
}

static int
cmd_newgroup(struct netmdcli_context *ctx)
{
    const char *group_name = netmdcli_context_get_string_arg(ctx, "group_name");

    netmd_create_group(ctx->devh, ctx->md, group_name);

    return 0;
}

static int
cmd_retitle(struct netmdcli_context *ctx)
{
    int group_id = netmdcli_context_get_int_arg(ctx, "group_id");
    const char *new_title = netmdcli_context_get_string_arg(ctx, "new_title");

    netmd_set_group_title(ctx->devh, ctx->md, (unsigned int)group_id, new_title);

    return 0;
}

static int
cmd_pause(struct netmdcli_context *ctx)
{
    netmd_pause(ctx->devh);

    return 0;
}

static int
cmd_stop(struct netmdcli_context *ctx)
{
    netmd_stop(ctx->devh);

    return 0;
}

static int
cmd_restart(struct netmdcli_context *ctx)
{
    netmd_track_restart(ctx->devh);

    return 0;
}

static int
cmd_next(struct netmdcli_context *ctx)
{
    netmd_track_next(ctx->devh);

    return 0;
}

static int
cmd_previous(struct netmdcli_context *ctx)
{
    netmd_track_previous(ctx->devh);

    return 0;
}

static int
cmd_fforward(struct netmdcli_context *ctx)
{
    netmd_fast_forward(ctx->devh);

    return 0;
}

static int
cmd_rewind(struct netmdcli_context *ctx)
{
    netmd_rewind(ctx->devh);

    return 0;
}

static int
cmd_json(struct netmdcli_context *ctx)
{
    json_object *json = json_object_new_object();
    json_object_object_add(json, "device",  json_object_new_string(ctx->netmd->device_name));
    json_object_object_add(json, "title",   json_object_new_string(netmd_minidisc_get_disc_name(ctx->md)));
    print_disc_info(ctx->devh, ctx->md, json);

    return 0;
}

static int
cmd_capacity(struct netmdcli_context *ctx)
{
    netmd_disc_capacity capacity;
    netmd_get_disc_capacity(ctx->devh, &capacity);

    char *recorded = netmd_time_to_string(&capacity.recorded);
    char *total = netmd_time_to_string(&capacity.total);
    char *available = netmd_time_to_string(&capacity.available);

    printf("Recorded:  %s (total play time)\n", recorded);
    printf("Total:     %s (SP units)\n", total);
    printf("Available: %s (SP units)\n", available);

    netmd_free_string(recorded);
    netmd_free_string(total);
    netmd_free_string(available);

    return 0;
}

static int
cmd_discinfo(struct netmdcli_context *ctx)
{
    netmd_device *netmd = ctx->netmd;
    minidisc *md = ctx->md;
    netmd_dev_handle *devh = ctx->devh;

    struct netmd_track_info info;

    uint8_t i = 0;
    uint8_t lastgroup = 0;

    netmd_disc_capacity capacity;
    netmd_get_disc_capacity(ctx->devh, &capacity);

    char *recorded = netmd_time_to_string(&capacity.recorded);
    char *total = netmd_time_to_string(&capacity.total);
    char *available = netmd_time_to_string(&capacity.available);

    int total_capacity = netmd_time_to_seconds(&capacity.total);
    int available_capacity = netmd_time_to_seconds(&capacity.available);

    // Need to calculate it like this, because only "total" and "available" are
    // calculated in SP time ("recorded" might be more than that due to Mono/LP2/LP4).
    int percentage_used = (total_capacity != 0) ? (100 * (total_capacity - available_capacity) / total_capacity) : 0;

    netmd_time_double(&capacity.available);
    char *available_lp2_mono = netmd_time_to_string(&capacity.available);

    netmd_time_double(&capacity.available);
    char *available_lp4 = netmd_time_to_string(&capacity.available);

    printf("\n");
    printf("  NetMD device: %s\n", netmd->device_name);
    printf("  Disc title:   %s\n", netmd_minidisc_get_disc_name(ctx->md));
    printf("  Recorded:     %s (%3d %% of %s used)\n", recorded, percentage_used, total);
    printf("  Available:    %s (SP) / %s (LP2/Mono) / %s (LP4)\n", available, available_lp2_mono, available_lp4);
    printf("\n");

    netmd_free_string(recorded);
    netmd_free_string(total);
    netmd_free_string(available);
    netmd_free_string(available_lp2_mono);
    netmd_free_string(available_lp4);

    printf("%s  Trk Name                                               Flags  Mode  MM:SS.tt%s\n",
            ansi_header_begin, ansi_end);

    for(i = 0; ; i++) {
        netmd_error res = netmd_get_track_info(devh, i, &info);

        if (res == NETMD_TRACK_DOES_NOT_EXIST) {
            break;
        } else if (res != NETMD_NO_ERROR) {
            fprintf(stderr, "Could not get track info for track %d: %s\n", i, netmd_strerror(res));
            continue;
        }

        int group = netmd_minidisc_get_track_group(md, i);
        int next_group = netmd_minidisc_get_track_group(md, i+1);

        /* Different to the last group? */
        if( group != lastgroup )
        {
            lastgroup = group;
            if( group )			/* Group 0 is 'no group' */
            {
                printf("%s%s (group %d)%s\n", ansi_grp_begin, netmd_minidisc_get_group_name(md, group), group, ansi_end);
            }
        }

        char *duration = netmd_track_duration_to_string(&info.duration);

        const char *ansi_encoding_begin = ansi_sp_begin;

        if (info.channels == NETMD_CHANNELS_MONO) {
            ansi_encoding_begin = ansi_mono_begin;
        } else if (info.encoding == NETMD_ENCODING_LP2) {
            ansi_encoding_begin = ansi_lp2_begin;
        } else if (info.encoding == NETMD_ENCODING_LP4) {
            ansi_encoding_begin = ansi_lp4_begin;
        }

        const char *ansi_title_begin = "";
        const char *ansi_title_end = "";
        if (strlen(info.title) == 0) {
            info.title = "<no title>";
            ansi_title_begin = ansi_no_title_begin;
            ansi_title_end = ansi_end;
        }

        // TODO: Take COLUMNS into account, do not hardcode 50-character title width
        printf("%s%s%s %s%02i%s %s%-50s%s %-6s %s%-4s%s %9s\n",
               ansi_grp_begin, (group != 0) ? ((next_group != group) ? "|_" : "| ") : "  ", ansi_end,
               ansi_trk_begin, i, ansi_end,
               ansi_title_begin, info.title, ansi_title_end,
               netmd_track_flags_to_string(info.protection),
               ansi_encoding_begin, netmd_get_encoding_name(info.encoding, info.channels), ansi_end,
               duration);

        netmd_free_string(duration);
    }

    for (int group=1; group < md->group_count; group++)
    {
        if (netmd_minidisc_is_group_empty(md, group)) {
            printf("%s%s (group %d, empty)%s\n",
                    ansi_grp_begin,
                    netmd_minidisc_get_group_name(md, group), group,
                    ansi_end);
        }
    }

    printf("\n");

    return 0;
}

static int
cmd_delete(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_int_arg(ctx, "track_id");
    int end_track_id = netmdcli_context_get_optional_int_arg(ctx, "end_track_id");

    if (end_track_id == -1) {
        end_track_id = track_id;
    }

    if (end_track_id < track_id || track_id >= 0xffff || end_track_id >= 0xffff) {
        netmdcli_print_argument_error(ctx, "Invalid track range specified: %d-%d",
                track_id, end_track_id);
        return 1;
    }

    netmd_cache_toc(ctx->devh);

    for (int track=end_track_id; track >= track_id && track >= 0; track--) {
        netmd_log(NETMD_LOG_VERBOSE, "delete: removing track %d\n", track);

        netmd_delete_track(ctx->devh, ((uint16_t) track) & 0xffff);
        netmd_wait_for_sync(ctx->devh);
    }

    netmd_sync_toc(ctx->devh);

    return 0;
}

static int
cmd_erase(struct netmdcli_context *ctx)
{
    const char *force = netmdcli_context_get_string_arg(ctx, "force");

    if (strcmp(force, "force") != 0) {
        netmdcli_print_argument_error(ctx, "Pass in 'force' as second argument");
        return 1;
    }

    netmd_log(NETMD_LOG_VERBOSE, "erase: executing erase\n");
    netmd_erase_disc(ctx->devh);

    return 0;
}

static int
cmd_settitle(struct netmdcli_context *ctx)
{
    const char *new_title = netmdcli_context_get_string_arg(ctx, "new_title");

    netmd_cache_toc(ctx->devh);
    netmd_set_disc_title(ctx->devh, new_title);
    netmd_sync_toc(ctx->devh);

    return 0;
}

static int
cmd_m3uimport(struct netmdcli_context *ctx)
{
    const char *filename = netmdcli_context_get_string_arg(ctx, "filename");

    import_m3u_playlist(ctx->devh, filename);

    return 0;
}

static void
on_send_progress(struct netmd_send_progress *send_progress)
{
    fprintf(stderr, "\r\033[K%s (%.0f %%)", send_progress->message, 100.f * send_progress->progress);
    fflush(stderr);
}

static int
cmd_send(struct netmdcli_context *ctx)
{
    const char *filename = netmdcli_context_get_string_arg(ctx, "filename");
    const char *title = netmdcli_context_get_optional_string_arg(ctx, "title");

    int result = netmd_send_track(ctx->devh, filename, title, on_send_progress, NULL);

    if (result == NETMD_NO_ERROR) {
        fprintf(stderr, "\r\033[KTransfer of %s completed successfully.\n", filename);
    } else {
        fprintf(stderr, "\r\033[Knetmd_send_track() failed: %s (%d)\n", netmd_strerror(result), result);
    }

    return (result == NETMD_NO_ERROR) ? 0 : 1;
}

static int
cmd_raw(struct netmdcli_context *ctx)
{
    const char *hexstring = netmdcli_context_get_string_arg(ctx, "hex_string");

    send_raw_message(ctx->devh, hexstring);

    return 0;
}

static int
cmd_setplaymode(struct netmdcli_context *ctx)
{
    const char *playmode = netmdcli_context_get_string_arg(ctx, "playmode");
    const char *arg2 = netmdcli_context_get_optional_string_arg(ctx, "arg2");
    const char *arg3 = netmdcli_context_get_optional_string_arg(ctx, "arg3");

    const char *args[] = { playmode, arg2, arg3, NULL };

    uint16_t playmode_value = 0;

    const char **arg = args;
    while (*arg != NULL) {
        if (strcmp(*arg, "single") == 0) {
            playmode_value |= NETMD_PLAYMODE_SINGLE;
        } else if (strcmp(*arg, "repeat") == 0) {
            playmode_value |= NETMD_PLAYMODE_REPEAT;
        } else if (strcmp(*arg, "shuffle") == 0) {
            playmode_value |= NETMD_PLAYMODE_SHUFFLE;
        } else if (strcmp(*arg, "none") == 0) {
            // that's okay -- set zero play mode
        } else {
            netmdcli_print_argument_error(ctx, "Invalid play mode value: '%s' (valid values: single, repeat, shuffle, none)", *arg);
            return 1;
        }

        ++arg;
    }

    netmd_set_playmode(ctx->devh, playmode_value);

    return 0;
}

static int
cmd_status(struct netmdcli_context *ctx)
{
    netmd_track_index track;
    char buffer[256];
    netmd_time time;

    /* TODO: error checking */
    netmd_get_position(ctx->devh, &time);
    track = netmd_get_current_track(ctx->devh);
    if (track != NETMD_INVALID_TRACK) {
        netmd_request_title(ctx->devh, track, buffer, sizeof(buffer));
        printf("Current track: %s \n", buffer);
    }

    char *time_str = netmd_time_to_string(&time);
    printf("Current playback position: %s\n", time_str);
    netmd_free_string(time_str);

    return 0;
}

static void
on_recv_progress(struct netmd_recv_progress *recv_progress)
{
    fprintf(stderr, "\r\033[K%s (%.0f %%)", recv_progress->message, 100.f * recv_progress->progress);
    fflush(stderr);
}

static int
cmd_recv(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_int_arg(ctx, "track_id");
    const char *filename = netmdcli_context_get_optional_string_arg(ctx, "filename");

    int result = netmd_recv_track(ctx->devh, track_id, filename, on_recv_progress, NULL);

    if (result == NETMD_NO_ERROR) {
        fprintf(stderr, "\r\033[KUpload completed successfully.\n");
    } else {
        fprintf(stderr, "\r\033[Knetmd_recv_track() failed: %s (%d)\n", netmd_strerror(result), result);
    }

    return (result == NETMD_NO_ERROR) ? 0 : 1;
}

static int
cmd_leave(struct netmdcli_context *ctx)
{
    netmd_error error = netmd_secure_leave_session(ctx->devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(error));

    return 0;
}

static int
cmd_settime(struct netmdcli_context *ctx)
{
    int track_id = netmdcli_context_get_int_arg(ctx, "track_id");
    int hour = 0;
    int minute = netmdcli_context_get_int_arg(ctx, "min");
    int second = netmdcli_context_get_optional_int_arg(ctx, "sec");
    int frame = netmdcli_context_get_optional_int_arg(ctx, "frame");

    if (frame == -1) {
        frame = 0;
    }

    if (second == -1) {
        second = 0;
    }

    hour = minute / 60;
    minute = minute % 60;

    netmd_time time;
    time.hour = hour;
    time.minute = minute;
    time.second = second;
    time.frame = frame;

    netmd_set_time(ctx->devh, track_id, &time);

    return 0;
}

static const struct netmdcli_subcommand
CMDS[] = {
    { "discinfo",    cmd_discinfo,    "",                              "Print disc information" },
    { "status",      cmd_status,      "",                              "Print current track and position" },
    { "capacity",    cmd_capacity,    "",                              "Print disc used/available time info" },
    { "---",         NULL,            NULL,                            NULL },
    { "erase",       cmd_erase,       "<force>",                       "Erase the entire disc (pass in 'force' to actually do it)" },
    { "settitle",    cmd_settitle,    "<new_title>",                   "Set the complete disc title (with group information)" },
    { "---",         NULL,            NULL,                            NULL },
    { "rename",      cmd_rename,      "<track_id> <new_title>",        "Rename track <track_id> to <new_title>" },
    { "move",        cmd_move,        "<from_track_id> <to_track_id>", "Move <from_track_id> to <to_track_id>" },
    { "delete",      cmd_delete,      "<track_id> [end_track_id]",     "Delete track <track_id>, or a range (if [end_track_id] is given)" },
    { "---",         NULL,            NULL,                            NULL },
    { "send",        cmd_send,        "<filename> [title]",            "Send WAV (16-bit, 44.1 kHz -or- ATRAC3 LP2/LP4) audio to device" },
    { "recv",        cmd_recv,        "<track_id> [filename]",         "Upload a track from the NetMD device to a file (MZ-RH1 only)" },
    { "---",         NULL,            NULL,                            NULL },
    { "play",        cmd_play,        "[track_id]",                    "Start/resume playback (from [track_id] if set)" },
    { "pause",       cmd_pause,       "",                              "Pause playback" },
    { "stop",        cmd_stop,        "",                              "Stop playback" },
    { "setplaymode", cmd_setplaymode, "<playmode> [arg2] [arg3]",      "Set play mode (single, repeat, shuffle or none, up to 3 args)" },
    { "---",         NULL,            NULL,                            NULL },
    { "previous",    cmd_previous,    "",                              "Start playing the previous track" },
    { "next",        cmd_next,        "",                              "Start playing the next track" },
    { "restart",     cmd_restart,     "",                              "Start playing the current track from start" },
    { "settime",     cmd_settime,     "<track_id> <min> [sec] [frame]", "Seek to a position on the disc" },
    { "---",         NULL,            NULL,                            NULL },
    { "fforward",    cmd_fforward,    "",                              "Start fast forwarding" },
    { "rewind",      cmd_rewind,      "",                              "Start rewinding" },
    { "---",         NULL,            NULL,                            NULL },
    { "newgroup",    cmd_newgroup,    "<group_name>",                  "Create a new group named <group_name>" },
    { "retitle",     cmd_retitle,     "<group_id> <new_title>",        "Rename group <group_id> to <new_title>" },
    { "deletegroup", cmd_deletegroup, "<group_id>",                    "Delete group <group_id>, but not the tracks in it" },
    { "groupmove",   cmd_groupmove,   "<group_id> <start_track_id>",   "Make <group_id> start at track <start_track_id>" },
    { "group",       cmd_group,       "<track_id> <group_id>",         "Put track <track_id> into group <group_id>" },
    { "---",         NULL,            NULL,                            NULL },
    { "m3uimport",   cmd_m3uimport,   "<filename>",                    "Import track title info from M3U file <filename>" },
    { "json",        cmd_json,        "",                              "Output current disc information as JSON" },
    { "---",         NULL,            NULL,                            NULL },
    { "raw",         cmd_raw,         "<hex_string>",                  "Send raw command to player (hex string)" },
    { "leave",       cmd_leave,       "",                              "Call netmd_secure_leave_session()" },
    { NULL,          NULL,            NULL,                            NULL },
};

static void
disable_colors(void)
{
    ansi_grp_begin = "";
    ansi_trk_begin = "";
    ansi_sp_begin = "";
    ansi_mono_begin = "";
    ansi_lp2_begin = "";
    ansi_lp4_begin = "";
    ansi_no_title_begin = "";
    ansi_header_begin = "";
    ansi_end = "";
}

int main(int argc, char* argv[])
{
    netmd_dev_handle* devh;
    minidisc my_minidisc, *md = &my_minidisc;
    netmd_device *device_list, *netmd;
    char name[16];
    int c;
    netmd_error error;
    int exit_code = 0;

    /* by default, log only errors */
    netmd_set_log_level(NETMD_LOG_ERROR);

    /* parse options */
    while (1) {
        c = getopt(argc, argv, "tvn");
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
        case 'n':
            disable_colors();
            break;
        default:
            fprintf(stderr, "Unknown option '%c'\n", c);
            break;
        }
    }

    if (!isatty(STDOUT_FILENO)) {
        disable_colors();
    }

    /* update argv and argc after parsing options */
    argv = &argv[optind - 1];
    argc -= (optind - 1);

    /* don't require device init to show help */
    if (argc == 1 || (argc > 1 && strcmp("help", argv[1]) == 0))
    {
        usage();
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

    /* parse commands */
    enum NetMDCLIHandleResult res = netmdcli_handle(CMDS, argc, argv, netmd, devh, md);
    if (res == NETMDCLI_HANDLED) {
        exit_code = 0;
    } else if (res == NETMDCLI_ERROR) {
        exit_code = 1;
    } else {
        netmd_log(NETMD_LOG_ERROR, "Unknown command '%s'; use 'help' for list of commands\n", argv[1]);
        exit_code = 1;
    }



    netmd_clean_disc_info(md);
    netmd_close(devh);
    netmd_clean(&device_list);

    return exit_code;
}

void print_disc_info(netmd_dev_handle* devh, minidisc* md, json_object *json)
{
    uint8_t i = 0;

    struct netmd_track_info info;

    netmd_disc_capacity capacity;
    netmd_get_disc_capacity(devh, &capacity);

    json_object_object_add(json, "recordedTime", json_time(&capacity.recorded));
    json_object_object_add(json, "totalTime", json_time(&capacity.total));
    json_object_object_add(json, "availableTime", json_time(&capacity.available));
    json_object* tracks = json_object_new_array();

    for(i = 0; ; i++)
    {
        netmd_error res = netmd_get_track_info(devh, i, &info);
        if (res == NETMD_TRACK_DOES_NOT_EXIST) {
            break;
        } else if (res != NETMD_NO_ERROR) {
            fprintf(stderr, "Could not get track info for track %d: %s\n", i, netmd_strerror(res));
            continue;
        }

        // Format track time
        char *duration = netmd_track_duration_to_string(&info.duration);

        // Create JSON track object and add to array
        json_object* track = json_object_new_object();
        json_object_object_add(track, "no",         json_object_new_int(i));
        json_object_object_add(track, "protect",    json_object_new_string(netmd_track_flags_to_string(info.protection)));
        json_object_object_add(track, "bitrate",    json_object_new_string(netmd_get_encoding_name(info.encoding, info.channels)));
        json_object_object_add(track, "time",       json_object_new_string(duration));
        json_object_object_add(track, "name",       json_object_new_string(info.title));
        json_object_array_add(tracks, track);

        netmd_free_string(duration);
    }

    json_object_object_add(json, "tracks", tracks);
    printf("%s\n", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));

    // Clean up JSON object
    json_object_put(json);
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
        usage();
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

void usage()
{
    puts("\nNetMD command line tool");
    puts("Usage: netmd [options] command args\n");
    puts("Options:");
    puts("      -v show debug messages");
    puts("      -n [n]o color output");
    puts("      -t enable tracing of USB command and response data\n");
    puts("Commands:\n");

    const struct netmdcli_subcommand *cmd = CMDS;
    while (cmd->name != NULL) {
        netmdcli_print_help(cmd);
        ++cmd;
    }

    puts("");
    puts("    usbids                                   Output NetMD USB ID list in JSON format");
    puts("    help                                     Show this message\n");

    puts("\nNote: Track numbers are currently zero-based.\n");

}
