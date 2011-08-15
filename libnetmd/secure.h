#ifndef LIBNETMD_SECURE_H
#define LIBNETMD_SECURE_H

#include <stdint.h>
#include "common.h"
#include "error.h"

/*
  linked list to store a list of 16-byte keys
*/
typedef struct netmd_keychain {
    char *key;
    struct netmd_keychain *next;
} netmd_keychain;

/*
  enabling key block

  @param id The ID of the EKB.
  @param chain A chain of encrypted keys. The one end of the chain is the
  encrypted root key, the other end is a key encrypted by a key the device has
  in it's key set. The direction of the chain is not yet known.
  @param depth Selects which key from the devices keyset has to be used to start
  decrypting the chain. Each key in the key set corresponds to a specific depth
  in the tree of device IDs.
  @param signature Signature of the root key (24 byte). Used to verify
  integrity of the decrypted root key by the device.
*/
typedef struct {
    uint32_t id;
    netmd_keychain *chain;
    uint32_t depth;
    char *signature;
} netmd_ekb;

typedef enum {
    NETMD_WIREFORMAT_PCM = 0,
    NETMD_WIREFORMAT_105KBPS = 0x90,
    NETMD_WIREFORMAT_LP2 = 0x94,
    NETMD_WIREFORMAT_LP4 = 0xa8
} netmd_wireformat;

/*
  Enter a session secured by a root key found in an EKB. The EKB for this
  session has to be download after entering the session.
*/
netmd_error netmd_secure_enter_session(netmd_dev_handle *dev);

/*
  Forget the key material from the EKB used in the secure session.
*/
netmd_error netmd_secure_leave_session(netmd_dev_handle *dev);

/*
  Read the leaf ID of the present NetMD device. The leaf ID tells which keys the
  device posesses, which is needed to find out which parts of the EKB needs to
  be sent to the device for it to decrypt the root key.
*/
netmd_error netmd_secure_get_leaf_id(netmd_dev_handle *dev, uint64_t *player_id);

/*
  Send key data to the device. The device uses it's builtin key
  to decrypt the root key from an EKB.
*/
netmd_error netmd_secure_send_key_data(netmd_dev_handle *dev, netmd_ekb *ekb);

/*
  Exchange a session key with the device. Needs to have a root key sent to the
  device using sendKeyData before.

  @param rand_in 8 bytes random binary data
  @param rand_out device nonce, another 8 bytes random data
*/
netmd_error netmd_secure_session_key_exchange(netmd_dev_handle *dev,
                                              unsigned char *rand_in,
                                              unsigned char *rand_out);

/*
  Invalidate the session key established by nonce exchange. Does not invalidate
  the root key set up by sendKeyData.
*/
netmd_error netmd_secure_session_key_forget(netmd_dev_handle *dev);

/*
  Prepare the download of a music track to the device.

  @param contentid 20 bytes Unique Identifier for the DRM system.
  @param keyenckey 8 bytes DES key used to encrypt the block data keys
  @param sessionkey 8 bytes DES key used for securing the current session, the
                    key has to be calculated by the caller from the data
                    exchanged in sessionKeyExchange and the root key selected by
                    sendKeyData
*/
netmd_error netmd_secure_setup_download(netmd_dev_handle *dev,
                                        unsigned char *contentid,
                                        unsigned char *key_encryption_key,
                                        unsigned char *sessionkey);

int netmd_secure_cmd_28(netmd_dev_handle *dev, unsigned int track_type, unsigned int length_byte,
                        unsigned int length, unsigned int *track_nr);

/*
  Commit a track. The idea is that this command tells the device hat the license
  for the track has been checked out from the computer.

  @param track  Track number returned from downloading command
  @param sessionkey 8-byte DES key used for securing the download session
*/
netmd_error netmd_secure_commit_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char *sessionkey);

/*
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

/*
  Secure delete with 8-byte signature?

  \param track track number to delete
  \param signature 8-byte signature of deleted track
*/
netmd_error netmd_secure_delete_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char *signature);


#endif
