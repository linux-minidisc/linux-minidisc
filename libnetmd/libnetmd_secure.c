/*
	Copyright 2004 Bertrik Sikken (bertrik@zonnet.nl)

	Set of NetMD commands that start with the sequence
	0x00,0x18,0x00,0x08,0x00,0x46,0xf0,0x03,0x01,0x03

	These commands are used during check-in/check-out.
*/

#include <string.h>
#include <usb.h>

#include "libnetmd.h"


#define	SECURE_CMD_HDR	0x00,0x18,0x00,0x08,0x00,0x46,0xf0,0x03,0x01,0x03


/** Helper function to make life a little simpler for other netmd_secure_cmd_* functions */
static int exch_secure_msg(netmd_dev_handle *dev, unsigned char *cmd, int cmdlen, unsigned char *rsp)
{
	int len;

	len = netmd_exch_message(dev, cmd, cmdlen, rsp);
	if (len < 0) {
		fprintf(stdout, "Exchange failed %d\n", len);
		return len;
	}
	switch (rsp[0]) {
	case 0x08:
		fprintf(stderr, "Command rejected\n");
		return NETMDERR_CMD_FAILED;
	case 0x09:
		/* command accepted */
		break;
	case 0x0a:
		fprintf(stderr, "Command parameters invalid\n");
		return NETMDERR_CMD_INVALID;
	case 0x0f:
		/* command accepted, now send bulk data */
		break;
	default:
		fprintf(stderr, "Unknown response code 0x%02X\n", rsp[0]);
		return NETMDERR_CMD_FAILED;
	}
	return len;
}


/************************************************************
	Functions common for check-in and check-out
*/

/** Start secure session? */
int netmd_secure_cmd_80(netmd_dev_handle *dev)
{
	unsigned char cmd[] = {SECURE_CMD_HDR, 0x80, 0xff};
	unsigned char rsp[255];

	return exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
}


/** Get 4-byte player ID? */
int netmd_secure_cmd_11(netmd_dev_handle *dev, unsigned int *player_id)
{
	unsigned char cmd[] = {SECURE_CMD_HDR, 0x11, 0xff};
	unsigned char rsp[255];
	int ret;

	ret = exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
	if (ret > 0) {
		*player_id = (
			(rsp[14] << 24) |
			(rsp[15] << 16) |
			(rsp[16] << 8) |
			(rsp[17] << 0));
	}
	return ret;
}


/** Send 40-byte EKB (enabling key block)? */
int netmd_secure_cmd_12(netmd_dev_handle *dev, unsigned char *ekb_head, unsigned char *ekb_body)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x12, 0xff, 0x00, 0x38, 0x00, 0x00,
		0x00, 0x38, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x09, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x00};
	unsigned char cmd[sizeof(cmdhdr) + 40];
	unsigned char rsp[255];
	unsigned char *buf;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	memcpy(buf, ekb_head, 16);
	buf += 16;
	memcpy(buf, ekb_body, 24);
	buf += 24;

	return exch_secure_msg(dev, cmd, buf - cmd, rsp);
}


/** Exchange 8-byte random? */
int netmd_secure_cmd_20(netmd_dev_handle *dev, unsigned char *rand_in, unsigned char *rand_out)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x20, 0xff, 0x00, 0x00, 0x00};
	unsigned char cmd[sizeof(cmdhdr) + 8];
	unsigned char rsp[255];
	unsigned char *buf;
	int ret;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	memcpy(buf, rand_in, 8);
	buf += 8;

	ret = exch_secure_msg(dev, cmd, buf - cmd, rsp);
	if (ret > 0) {
		memcpy(rand_out, rsp + sizeof(cmdhdr), 8);
	}

	return ret;
}


/** Discard random? */
int netmd_secure_cmd_21(netmd_dev_handle *dev)
{
	unsigned char cmd[] = {SECURE_CMD_HDR, 0x21, 0xff, 0x00, 0x00, 0x00};
	unsigned char rsp[255];

	return exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
}


/** End secure session? */
int netmd_secure_cmd_81(netmd_dev_handle *dev)
{
	unsigned char cmd[] = {SECURE_CMD_HDR, 0x81, 0xff};
	unsigned char rsp[255];

	return exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
}


/************************************************************
	Functions related to check-out
*/

/** Send 32-byte hash? */
int netmd_secure_cmd_22(netmd_dev_handle *dev, unsigned char *hash)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x22, 0xff, 0x00, 0x00};
	unsigned char cmd[sizeof(cmdhdr) + 32];
	unsigned char rsp[255];
	unsigned char *buf;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	memcpy(buf, hash, 32);
	buf += 32;

	return exch_secure_msg(dev, cmd, buf - cmd, rsp);
}


/** Prepare USB download?
	\param dev USB device handle
	\param track_type SP=0x006, LP2=0x9402, LP4=0xA800
	\param length_byte ??? 0x58 and 0x5A have been seen here
	\param length length of USB download
	\param *track_nr new track number
*/
int netmd_secure_cmd_28(netmd_dev_handle *dev, unsigned int track_type, unsigned int length_byte,
	unsigned int length, unsigned int *track_nr)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x28, 0xff, 0x00, 0x01, 0x00, 0x10,
		0x01, 0xff, 0xff, 0x00};
	unsigned char cmd[30];
	unsigned char rsp[255];
	unsigned char *buf;
	int ret;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	*buf++ = (track_type >> 8) & 0xFF;
	*buf++ = (track_type >> 0) & 0xFF;
	*buf++ = 0;
	*buf++ = 0;
	*buf++ = (length_byte >> 8) & 0xFF;
	*buf++ = (length_byte >> 0) & 0xFF;
	*buf++ = (length >> 24) & 0xFF;
	*buf++ = (length >> 16) & 0xFF;
	*buf++ = (length >> 8) & 0xFF;
	*buf++ = (length >> 0) & 0xFF;

	ret = exch_secure_msg(dev, cmd, buf - cmd, rsp);
	if (ret > 0) {
		*track_nr = (rsp[17] << 8) | rsp[18];
	}
	return ret;
}


/** Verify track with 8-byte hash? */
int netmd_secure_cmd_48(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *hash)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x48, 0xff, 0x00, 0x10, 0x01};
	unsigned char cmd[sizeof(cmdhdr) + 10];
	unsigned char rsp[255];
	unsigned char *buf;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	*buf++ = (track_nr >> 8) & 0xFF;
	*buf++ = (track_nr >> 0) & 0xFF;
	memcpy(buf, hash, 8);
	buf += 8;

	return exch_secure_msg(dev, cmd, buf - cmd, rsp);
}


/************************************************************
	Functions related to check-in
*/

/** Get 8-byte hash id of a track? */
int netmd_secure_cmd_23(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *hash_id)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x23, 0xff, 0x10, 0x01};
	unsigned char cmd[sizeof(cmdhdr) + 2];
	unsigned char rsp[255];
	unsigned char *buf;
	int ret;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	*buf++ = (track_nr >> 8) & 0xFF;
	*buf++ = (track_nr >> 0) & 0xFF;

	ret = exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
	if (ret > 0) {
		memcpy(hash_id, rsp + 16, 8);
	}
	return ret;
}


/** Secure delete with 8-byte signature?
	\param dev USB device handle
	\param track_nr track number to delete?
	\param signature 8-byte signature of deleted track
*/
int netmd_secure_cmd_40(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *signature)
{
	unsigned char cmdhdr[] = {SECURE_CMD_HDR, 0x40, 0xff, 0x00, 0x10, 0x01};
	unsigned char cmd[sizeof(cmdhdr) + 2];
	unsigned char rsp[255];
	unsigned char *buf;
	int ret;

	buf = cmd;
	memcpy(buf, cmdhdr, sizeof(cmdhdr));
	buf += sizeof(cmdhdr);
	*buf++ = (track_nr >> 8) & 0xFF;
	*buf++ = (track_nr >> 0) & 0xFF;

	ret = exch_secure_msg(dev, cmd, sizeof(cmd), rsp);
	if (ret > 0) {
		memcpy(signature, rsp + 17, 8);
	}
	return ret;
}


