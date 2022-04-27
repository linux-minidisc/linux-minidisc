Minidisc internals documentation
--------------------------------

This directory contains documents related to MD player protocol
internals.

**The documents in this directory may have a different license
than other files in this repository.**

## AV/C standards (NetMD)

The AV/C standards documents specify the protocol used to
control NetMD devices, with the exception of some vendor-specific
commands used to transfer audio tracks.

A sample USB command packet with references to the documents in this
directory to get started with investigating the protocol:

```
Track title write (excludes descriptor open/close commands):

0020   xx xx xx xx 00 18 07 02 20 18 02 00 00 30 00 0a
                   CT SU OP Dc Ds D1 D2 D3 D4 Is  It
0030   00 50 00 00 0a 00 00 00 00 6d 61 74 73 2d 31 30
       Ii SF    L1 L2             track-name----------
0040   73 65 63
       --------

xx = USB framing bytes, not relevant

CT = command type (ctype): usually 0 for commands from the host (Digital Interface Commands 5.1)
     (this field will be the response code in messages from the devices, usually 9 for success)

  lower 4 bits of first byte (upper 4 bits always zero), 0 = CONTROL

SU = "subunit" aka target device for this command, generally 0x18 for MD (Digital Interface Commands 5.3.3)

  0x18 means "subunit type 3, subunit ID 0"

           __ subunit ID, here 0b000 = 0 (0..4 = instance number, see "Subunit ID encoding")
          /
         / \
  0b00011000 = 0x18
    \___/
      \__ subunit_type, here 0b00011 = 3 = disc recorder/player (audio or video)

OP = top-level opcode: WRITE INFO BLOCK (Descriptor Mechanism 9.9)

The following fields describe the target of the operation viz. the
text "infoblock" beloging to the "text database" (id 18 02) entry for
the first track (track 0):

Dc = descriptor level count (Descriptor Mechanism 8.3.1)
Ds = descriptor type specifier, 0x20 = index in list (Descriptor Mechanism 8.1, 8.2.4)
D1-2 (18 02) = target list id: track title descriptor list (MD Audio 8.3.1)
D3-4 = index in target list (in this case the track number)
Is = descriptor type specifier 2, 0x30 = info block (Descriptor Mechanism 8.4)
It = info block type (two bytes): "raw text info" (MD Audio B.4 example)
Ii = info block instance count

Finally the details of the write operation:

SF = WRITE INFO BLOCK "partial replace" subfunction (Descriptor Mechanism 9.9)
L1-2 = new value length (0xa = 10 bytes)

"Track name" contains the actual track name with the length as specified by
L1-2 (0x000a = 10 bytes in this case).
````
