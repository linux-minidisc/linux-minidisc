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

int netmd_secure_session_key_exchange(netmd_dev_handle *dev,
                                      uint64_t rand_in,
                                      uint64_t *rand_out);

int netmd_secure_session_key_forget(netmd_dev_handle *dev);

int netmd_secure_cmd_22(netmd_dev_handle *dev, unsigned char *hash);
int netmd_secure_cmd_28(netmd_dev_handle *dev, unsigned int track_type, unsigned int length_byte,
                        unsigned int length, unsigned int *track_nr);
int netmd_secure_cmd_48(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *hash);
int netmd_secure_cmd_23(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *hash_id);
int netmd_secure_cmd_40(netmd_dev_handle *dev, unsigned int track_nr, unsigned char *signature);


#endif
