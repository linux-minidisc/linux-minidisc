#ifndef CONST_H
#define CONST_H

/**
   Error codes of the USB transport layer
*/
#define NETMDERR_USB -1         /**< general USB error */
#define NETMDERR_NOTREADY -2    /**< player not ready for command */
#define NETMDERR_TIMEOUT -3     /**< timeout while waiting for response */
#define NETMDERR_CMD_FAILED -4  /**< minidisc responded with 08 response */
#define NETMDERR_CMD_INVALID -5 /**< minidisc responded with 0A response */

/**
   Playmode values to be sent to netmd_set_playmode. These can be combined by
   OR-ing them to do shuffle repeat for example.

   See also: http://article.gmane.org/gmane.comp.audio.netmd.devel/848
*/
#define NETMD_PLAYMODE_SINGLE 0x0040
#define NETMD_PLAYMODE_REPEAT 0x0080
#define NETMD_PLAYMODE_SHUFFLE 0x0100

/**
   playback controll commands
*/
#define NETMD_PLAY 0x75
#define NETMD_PAUSE 0x7d
#define NETMD_REWIND 0x49
#define NETMD_FFORWARD 0x39

/**
   change track commands
*/
#define NETMD_TRACK_PREVIOUS 0x0002
#define NETMD_TRACK_NEXT 0x8001
#define NETMD_TRACK_RESTART 0x0001

/**
   NetMD Protocol return status (first byte of request)
*/
#define NETMD_STATUS_CONTROL 0x00
#define NETMD_STATUS_STATUS 0x01
#define NETMD_STATUS_SPECIFIC_INQUIRY 0x02
#define NETMD_STATUS_NOTIFY 0x03
#define NETMD_STATUS_GENERAL_INQUIRY 0x04

/**
   NetMD Protocol return status (first byte of response)
*/
#define NETMD_STATUS_NOT_IMPLEMENTED 0x08
#define NETMD_STATUS_ACCEPTED 0x09
#define NETMD_STATUS_REJECTED 0x0a
#define NETMD_STATUS_IN_TRANSITION 0x0b
#define NETMD_STATUS_IMPLEMENTED 0x0c
#define NETMD_STATUS_CHANGED 0x0d
#define NETMD_STATUS_INTERIM 0x0f

#define NETMD_ENCODING_SP 0x90
#define NETMD_ENCODING_LP2 0x92
#define NETMD_ENCODING_LP4 0x93

#define NETMD_CHANNELS_MONO 0x01
#define NETMD_CHANNELS_STEREO 0x00

#define NETMD_OPERATING_STATUS_USB_RECORDING 0x56ff
#define NETMD_OPERATING_STATUS_RECORDING 0xc275
#define NETMD_OPERATING_STATUS_RECORDING_PAUSED 0xc27d
#define NETMD_OPERATING_STATUS_FAST_FORWARDING 0xc33f
#define NETMD_OPERATING_STATUS_REWINDING 0xc34f
#define NETMD_OPERATING_STATUS_PLAYING 0xc375
#define NETMD_OPERATING_STATUS_PAUSED 0xc37d
#define NETMD_OPERATING_STATUS_STOPPED 0xc5ff

#define NETMD_TRACK_FLAG_PROTECTED 0x03

#define NETMD_DISC_FLAG_WRITABLE 0x10
#define NETMD_DISC_FLAG_WRITE_PROTECTED 0x40

#define NETMD_DISKFORMAT_LP4 0
#define NETMD_DISKFORMAT_LP2 2
#define NETMD_DISKFORMAT_SP_MONO 4
#define NETMD_DISKFORMAT_SP_STEREO 6

#define NETMD_RIFF_FORMAT_TAG_ATRAC3 0x270
#define NETMD_DATA_BLOCK_SIZE_LP2 384
#define NETMD_DATA_BLOCK_SIZE_LP4 192

#endif
