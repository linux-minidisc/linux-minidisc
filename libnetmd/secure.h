#ifndef LIBNETMD_SECURE_H
#define LIBNETMD_SECURE_H

/** \file secure.h */

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "error.h"

/**
   linked list to store a list of 16-byte keys
*/
typedef struct netmd_keychain {
    char *key;
    struct netmd_keychain *next;
} netmd_keychain;

/**
   enabling key block
*/
typedef struct {
    /** The ID of the EKB. */
    uint32_t id;

    /** A chain of encrypted keys. The one end of the chain is the encrypted
       root key, the other end is a key encrypted by a key the device has in
       it's key set. The direction of the chain is not yet known. */
    netmd_keychain *chain;

    /** Selects which key from the devices keyset has to be used to start
       decrypting the chain. Each key in the key set corresponds to a specific
       depth in the tree of device IDs. */
    uint32_t depth;

    /** Signature of the root key (24 byte). Used to verify integrity of the
       decrypted root key by the device. */
    char *signature;
} netmd_ekb;

/**
   linked list, storing all information of the single packets, send to the device
   while uploading a track
*/
typedef struct netmd_track_packets {
    /** encrypted key for this packet (8 bytes) */
    unsigned char *key;

    /** IV for the encryption  (8 bytes) */
    unsigned char *iv;

    /** the packet data itself */
    unsigned char *data;

    /** length of the data */
    size_t length;

    /** next packet to transfer (linked list) */
    struct netmd_track_packets *next;
} netmd_track_packets;

/**
   Format of the song data packets, that are transfered over USB.
*/
typedef enum {
    NETMD_WIREFORMAT_PCM = 0,
    NETMD_WIREFORMAT_105KBPS = 0x90,
    NETMD_WIREFORMAT_LP2 = 0x94,
    NETMD_WIREFORMAT_LP4 = 0xa8
} netmd_wireformat;

/**
   Enter a session secured by a root key found in an EKB. The EKB for this
   session has to be download after entering the session.
*/
netmd_error netmd_secure_enter_session(netmd_dev_handle *dev);

/**
   Forget the key material from the EKB used in the secure session.
*/
netmd_error netmd_secure_leave_session(netmd_dev_handle *dev);

/**
   Read the leaf ID of the present NetMD device. The leaf ID tells which keys the
   device posesses, which is needed to find out which parts of the EKB needs to
   be sent to the device for it to decrypt the root key.
*/
netmd_error netmd_secure_get_leaf_id(netmd_dev_handle *dev, uint64_t *player_id);

/**
   Send key data to the device. The device uses it's builtin key
   to decrypt the root key from an EKB.
*/
netmd_error netmd_secure_send_key_data(netmd_dev_handle *dev, netmd_ekb *ekb);

/**
   Exchange a session key with the device. Needs to have a root key sent to the
   device using sendKeyData before.

   @param rand_in 8 bytes random binary data
   @param rand_out device nonce, another 8 bytes random data
*/
netmd_error netmd_secure_session_key_exchange(netmd_dev_handle *dev,
                                              unsigned char *rand_in,
                                              unsigned char *rand_out);

/**
   Invalidate the session key established by nonce exchange. Does not invalidate
   the root key set up by sendKeyData.
*/
netmd_error netmd_secure_session_key_forget(netmd_dev_handle *dev);

/**
   Prepare the download of a music track to the device.

   @param contentid 20 bytes Unique Identifier for the DRM system.
   @param keyenckey 8 bytes DES key used to encrypt the block data keys
   @param sessionkey 8 bytes DES key used for securing the current session, the
                     key has to be calculated by the caller from the data
                     exchanged in sessionKeyExchange and the root key selected
                     by sendKeyData.
*/
netmd_error netmd_secure_setup_download(netmd_dev_handle *dev,
                                        unsigned char *contentid,
                                        unsigned char *key_encryption_key,
                                        unsigned char *sessionkey);

/**
 * Struct containing send progress information for netmd_send_progress_func().
 */
struct netmd_send_progress {
    float progress; /* 0.0f -> 1.0f */
    const char *message; /* message to display to the user */
    void *user_data; /* opaque pointer passed in as closure */
};

/**
 * Progress function callback netmd_secure_send_track()
 *
 * @param progress Progress of the current task (0.0f -> 1.0f)
 * @param msg Message what's currently happening
 * @param user_data Closure pointer
 **/
typedef void (*netmd_send_progress_func)(struct netmd_send_progress *progress);

/**
   Send a track to the NetMD unit.

   @param wireformat Format of the packets that are transported over usb
   @param discformat Format of the song in the minidisc
   @param frames Number of frames we need to transfer. Framesize depends on the
                 wireformat.
   @param packets Linked list with all packets that are nessesary to transfer
                  the complete song.
   @param packet_count Count of the packets in the linked list.
   @param sessionkey 8 bytes DES key used for securing the current session,
   @param track Pointer to where the new track number should be written to after
                trackupload.
   @param uuid Pointer to 8 byte of memory where the uuid of the new track is
               written to after upload.
   @param content_id Pointer to 20 byte of memory where the content id of the
                     song is written to afte upload.
   @param send_progress Callback to be called for progress updates, or NULL
   @param send_progress_user_data Pointer to pass to send_progress(), or NULL
*/
netmd_error netmd_secure_send_track(netmd_dev_handle *dev,
                                    netmd_wireformat wireformat,
                                    unsigned char discformat,
                                    unsigned int frames,
                                    netmd_track_packets *packets,
                                    size_t packet_length,
                                    unsigned char *sessionkey,

                                    uint16_t *track, unsigned char *uuid,
                                    unsigned char *content_id,
                                    netmd_send_progress_func send_progress,
                                    void *send_progress_user_data);

/**
 * Struct containing send progress information for netmd_send_progress_func().
 */
struct netmd_recv_progress {
    float progress; /* 0.0f -> 1.0f */
    const char *message; /* message to display to the user */
    void *user_data; /* opaque pointer passed in as closure */
};

/**
 * Progress function callback netmd_secure_recv_track()
 *
 * @param progress Progress of the current task (0.0f -> 1.0f)
 * @param msg Message what's currently happening
 * @param user_data Closure pointer
 **/
typedef void (*netmd_recv_progress_func)(struct netmd_recv_progress *progress);


/**
 * Upload audio data from disc to PC (MZ-RH1 in NetMD mode only)
 *
 * @param dev Handle to device
 * @param track_id Zero-based track ID
 * @param file Opened file to write received data to
 */
netmd_error netmd_secure_recv_track(netmd_dev_handle *dev, uint16_t track_id,
                                    FILE* file,
                                    netmd_recv_progress_func recv_progress,
                                    void *recv_progress_user_data);


/**
   Commit a track. The idea is that this command tells the device hat the license
   for the track has been checked out from the computer.

   @param track  Track number returned from downloading command
   @param sessionkey 8-byte DES key used for securing the download session
*/
netmd_error netmd_secure_commit_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char *sessionkey);

/**
   Gets the DRM tracking ID for a track.
   NetMD downloaded tracks have an 8-byte identifier (instead of their content
   ID) stored on the MD medium. This is used to verify the identity of a track
   when checking in.

   @param track The track number
   @param uuid  Pointer to the memory, where the 8-byte uuid of the track sould
                be saved.
*/
netmd_error netmd_secure_get_track_uuid(netmd_dev_handle *dev, uint16_t track,
                                        unsigned char *uuid);

/**
   Secure delete with 8-byte signature?

   @param track track number to delete
   @param signature 8-byte signature of deleted track
*/
netmd_error netmd_secure_delete_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char *signature);

netmd_error netmd_prepare_packets(unsigned char* data, size_t data_lenght,
                                  netmd_track_packets **packets,
                                  size_t *packet_count, unsigned int *frames, size_t channels, size_t *packet_length,
                                  unsigned char *key_encryption_key, netmd_wireformat format);

void netmd_cleanup_packets(netmd_track_packets **packets);

netmd_error netmd_secure_set_track_protection(netmd_dev_handle *dev,
                                              unsigned char mode);

#endif
