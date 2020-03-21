# linux-minidisc NetMD-fixes-fork

This fork integrates work related to enhancing NetMD support from several people on
top of that done by the original [linux-minidisc](https://github.com/glaubitz/linux-minidisc)
project. I have only fixed a few issues and cleaned the code somewhat; nearly all of the
actual work was done by keepers of earlier forks and authors of PRs to the original
project. I'm putting this up with this additional README since the original
repo appears to be pretty quiet and it was rather hard to tell where to get the most
up-to-date NetMD support.

The result is that downloads (PC-to-Minidisc transfers) now work without artifacts
for both SP (ATRAC1) and LP2/LP4 (ATRAC3) tracks at least on an MZ-N710. The player's
USB interface will still occasionally crash, but things work most of the time.

Original README is [here](README). HiMD support should work, but since only oil barons
can afford HiMD players in 2018, I have not tested it.

SP tracks will be transferred as PCM and encoded on the device. LP2/LP4 tracks
must be encoded on the host into an ATRAC3 WAV file. Because there is no complete free
ATRAC3 encoder available as of early 2018, most users should probably use ATRAC1 (but
see below if you want to try your luck anyway).

## Building

Building the project excluding some of the HiMD parts requires `qmake`, `glib`, `libgcrypt`
and `libusb`.

### Ubuntu
On a recent Ubuntu, the following installs the required dependencies:

    apt-get install libgcrypt20-dev libglib2.0-dev libusb-1.0-0-dev qt4-qmake libid3tag0-dev libmad0-dev

To set up the Makefiles, run

    qmake -qt=qt4 CONFIG+=without_mad CONFIG+=without_gui

Next, run `make` and everything relevant should get built.

By default, the USB device will only be accessible as root. Therefore you may want
to run the following commands to automatically give members of the `plugdev` group access
to the NetMD device:

    sudo cp netmd/etc/netmd.rules /etc/udev/rules.d/99-netmd.rules
    sudo udevadm control -R

Also, add your own user to the `plugdev` group if not yet added.

### Fedora
On a recent Fedora, the following installs the required dependencies:

    dnf install libgcrypt-devel glib2-devel libusb-devel qt-devel ibid3tag-devel libmad-devel
    
To set up the Makefiles, run

    qmake-qt4 CONFIG=without_mad CONFIG+=without_gui

Next, run `make` and everything relevant should get built.

Follow the same instructions as Ubuntu for the `udev` rules, however the `plugdev` group will need to be added first:

    groupadd plugdev
    usermod -aG plugdev [username]

### MacOS
Building on MacOS should also work. You will need to have `pkg-config` installed.

## Usage

The relevant executable will appear in the `netmdcli` subdirectory after compilation.
If you did not install the udev rules, you will need to run the following commands as
root.

Run `./netmdcli` to see a list of tracks on the inserted disc. Running `./netmdcli help`
will show the available commands for functions such as controlling playback and editing
the TOC.

Groups are supported to some degree, but the way they are stored on the disc is utterly
horrible, so expect problems. Notably, the `move` and `delete` commands will not
update group members' track numbers.

To download tracks, convert them to 44.1kHz 16-bit WAV format and run

    ./netmdcli -v send <file.wav> [<track title>]

If track title is omitted, the file name will be used. Transfer rate is somewhat
under 2x.

### ATRAC3

The experimental ATRAC3 encoder available from https://github.com/dcherednik/atracdenc
can be used together with `ffmpeg` to generate downloadable WAV files. Compile the
encoder and run:

    atracdenc --encode atrac3 -i source.wav -o dest.oma
    ffmpeg -i dest.oma -c:a copy dest.wav
    netmdcli -v send dest.wav

### Note on track uploads

Because NetMD happened at the peak of music industry DRM madness, its limitations
are pretty bonkers. You cannot upload tracks (Minidisc-to-PC transfer) over USB unless
those tracks were originally downloaded over USB. For your own recordings,
you will either need to use an MZ-RH1 (the last Minidisc player ever released which removed
most restrictions), or if you are not yet a millionaire, record from the analog/SPDIF
outputs. The `netmd/dump_md.py` script may be helpful in the latter case.

If you care about upload support, there is some support in the code for it, but
it probably needs work.

## Troubleshooting
***Minidisc player keeps disconnecting on Linux* or, *Minidisc player spins up, says `PC->->MD`, and then resets***

This is likely due to having the `libmtp` library installed.
Some programs pull this in as a dependency to be able to open MTP media players, e.g.
VLC Media Player.

This library includes a `udev` rule that falls back to probing the device if it's
not matched in it's hardware DB. This probing program may cause the USB connection
of the Minidisc player to reset, thus cycling the connection.

Workarounds include uninstalling the library, creating a custom rule to work around the probe,
or comment it out from `/lib/udev/rules.d/69-libmpt.rules`:

    # Autoprobe vendor-specific, communication and PTP devices
    #ENV{ID_MTP_DEVICE}!="1", ENV{MTP_NO_PROBE}!="1", ENV{COLOR_MEASUREMENT...


## Things that would be nice to have

  * More reliability. Back-to-back TOC manipulation operations seem to cause the
    most trouble.
  * A better WAV parser. Reading track names from the `INFO` chunk would be nice
    and the current method of finding the `data` chunk is a bit fragile.
  * Less buggy group support, though when using SP this isn't much of an issue.
  * Support for compressed input formats and multi-file sends, though these may be
    better off implemented as a wrapper script.
  * Support for uploading all tracks over USB. This would require some sort of player
    firmware exploit. A great hacking challenge.
