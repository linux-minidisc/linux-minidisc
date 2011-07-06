#include "libnetmd.h"

#define NETMD_PLAY	0x75
#define NETMD_PAUSE	0x7d
#define NETMD_REWIND	0x49
#define NETMD_FFORWARD  0x39

#define NETMD_TRACK_PREVIOUS    0x0002
#define NETMD_TRACK_NEXT        0x8001
#define NETMD_TRACK_RESTART     0x0001

static int netmd_playback_control(netmd_dev_handle* dev, unsigned char code)
{
    char request[] = {0x00, 0x18, 0xc3, 0xff, 0x00, 0x00,
                      0x00, 0x00};
    char buf[255];
    int size;

    request[4] = code;
    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_play(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PLAY);
}

int netmd_pause(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PAUSE);
}

int netmd_fast_forward(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_FFORWARD);
}

int netmd_rewind(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_REWIND);
}

int netmd_stop(netmd_dev_handle* dev)
{
    char request[] = {0x00, 0x18, 0xc5, 0xff, 0x00, 0x00,
                      0x00, 0x00};
    char buf[255];
    int size;

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_set_playmode(netmd_dev_handle* dev, int playmode)
{
    char request[] = {0x00, 0x18, 0xd1, 0xff, 0x01, 0x00,
                      0x00, 0x00, 0x88, 0x08, 0x00, 0x00,
                      0x00};
    char buf[255];
    int size;

    request[10] = (playmode >> 8) & 0xFF;
    request[11] = (playmode >> 0) & 0xFF;

    size = netmd_exch_message(dev, request, sizeof(request), buf);
    return size;
}

int netmd_set_track( netmd_dev_handle* dev, int track)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x01, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int size;

    request[10] = track-1;

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_change_track(netmd_dev_handle* dev, int direction)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x10, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int size;

    request[9] = (direction >> 8) & 0xFF;
    request[10] = (direction >> 0) & 0xFF;

    size = netmd_exch_message(dev, request, sizeof(request), buf);
    return size;
}

int netmd_get_track(netmd_dev_handle* dev)
{
    char request[] = {0x00, 0x18, 0x09, 0x80, 0x01, 0x04,
                      0x30, 0x88, 0x02, 0x00, 0x30, 0x88,
                      0x05, 0x00, 0x30, 0x00, 0x03, 0x00,
                      0x30, 0x00, 0x02, 0x00, 0xff, 0x00,
                      0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int track = 0;

    netmd_exch_message(dev, request, 28, buf);
    track = buf[36];

    return track;
}

int netmd_track_next(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_NEXT);
}

int netmd_track_previous(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_PREVIOUS);
}

int netmd_track_restart(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_RESTART);
}

int netmd_set_time(netmd_dev_handle* dev, int track, const netmd_time* time)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00};
    char buf[255];
    int size;

    request[9] = (track >> 8) & 0xFF;
    request[10] = (track >> 0) & 0xFF;
    request[11] = proper_to_bcd(time->hour);
    request[12] = proper_to_bcd(time->minute);
    request[13] = proper_to_bcd(time->second);
    request[14] = proper_to_bcd(time->frame);

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

const netmd_time* netmd_get_position(netmd_dev_handle* dev, netmd_time* time)
{
    char request[] = {0x00, 0x18, 0x09, 0x80, 0x01, 0x04,
                      0x30, 0x88, 0x02, 0x00, 0x30, 0x88,
                      0x05, 0x00, 0x30, 0x00, 0x03, 0x00,
                      0x30, 0x00, 0x02, 0x00, 0xff, 0x00,
                      0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int ret = 0;

    ret = netmd_exch_message(dev, request, sizeof(request), buf);

    time->hour = bcd_to_proper(buf[37]);
    time->minute = bcd_to_proper(buf[38]);
    time->second = bcd_to_proper(buf[39]);
    time->frame = bcd_to_proper(buf[40]);

    return time;
}

