# minidisc-ffwd: friendly fork, wip development

A friendly fork of `linux-minidisc` to more freely experiment
with things with less friction while eventually getting most
changes merged back into the upstream repository.

[Upstream](https://en.wikipedia.org/wiki/Upstream_(software_development)) repos and relationships:

```
    (o) Original Upstream Repo ("upstream")
     |  https://github.com/linux-minidisc/linux-minidisc
     |
    (o) NetMD fixes by vuori (upstream PR#62)
     |  https://github.com/vuori/linux-minidisc
     |
    (o) Downstream repo w/ fixes, used by Platium MD (upstream PR#82)
     |  https://github.com/gavinbenda/linux-minidisc
     |
  > (o) This Repo
     |  https://github.com/thp/linux-minidisc
     |
     v
```

Ideally all those changes will eventually find their way back into the
upstream repo, but for now, this faster-moving branch is where it's at.

README of the vuori fork is [here](README.vuori).
Original README of upstream is [here](README).


## NetMD Download Support for QHiMDTransfer

Now that `netmd_send_track()` exists in `libnetmd`, we can call it from
QHiMDTransfer to download tracks to the device. This accepts the same
file formats as Platinum MD, namely 44.1 kHz 16-bit stereo WAV files for
SP download or ATRAC LP2/LP4 data stored in a WAV container for direct
LP2/LP4 download.


## Track Duration Fix

`libnetmd` returned the wrong duration for tracks > 1 hour, because the
"hour" item of the response wasn't take into account (so e.g. a 68-minute
track would show up as an 8-minute track). This is fixed now, and the
`struct netmd_track`'s `minute` can now be >= 60.


## Meson Build System

There is now support for the [Meson Build System](https://mesonbuild.com/),
which is a bit more flexible and modern than the old qmake (for libs, CLI).

Care was taken so that the old `qmake`-based system still works if needed.

Building with Meson is simple, it always builds out-of-source-tree (here,
`build` is used as the build folder, and `ninja` is the build system):

```
meson build
ninja -C build
```

To build `libnetmd` and `libhimd` as static libraries (e.g. so that
`netmdcli` and `himdcli` can be used standalone):

```
meson build -Ddefault_library=static
ninja -C build
```

As part of this modification, the `libnetmd` public headers do not depend
on `libusb.h` directly anymore, which helps keep things modular.

The build dependencies are now much cleaner, and Meson's dependency system
is used, which on Linux uses `pkg-config` for everything by default, which
works fine on modern distros.


## Linux NetMD Permissions

The udev rules file for Linux (so users in the "plugdev" group will have
access to NetMD devices without requiring root) is now installed in the
right place automatically when using the Meson build system.

The HAL file (`netmd/etc/20-netmd.fdi`) was removed without replacement,
as HAL is deprecated in Linux for more than 10 years now.


## libusbmd: Central VID/PID database

A new library "libusbmd" contains the central database for both NetMD and
Hi-MD USB device IDs. It provides a simple C API for querying all entries
as well as identifying a device given a VID and PID.

The CLI tools (`netmdcli` and `himdcli`) now each have a `usbids` sub-command
that will query `libusbmd` for the specific device IDs and output them as
JSON (VID and PID as decimal, as JSON doesn't support hex literals).

QHiMDTransfer now also uses `libusbmd` for identifying devices.

The database is embedded in `libusbmd` and generated from `devicedb/minidisc.devids`.


## JSON output format string vulnerability

When outputting the track list for both `netmdcli` and `himdcli`, there was
a format string vulnerability, as the JSON output was the format string passed
to `printf()`. Titling a track with special printf commands (`%s`, `%n`, ...)
would result in a crash or worse.


## Removal of qmake support

The `meson.build` file now has support for building the Qt5 GUI, so we can remove
all the `qmake`-related cruft. By default the GUI building is disabled, but you
can enable it with:

```
meson build -Dwith_gui=true
ninja -C build
build/qhimdtransfer
```

Some things might have been broken during the build system transition, let me know
and we can look into fixing it up.


## Proper device type detection in QHiMDTransfer

Use the device type provided by `libusbmd` for detection of devices instead of
relying on the string "NetMD" appearing in the device name.


## Nicer command-line handling for netmdcli

More structured command-line handling and better argument parsing / error reporting
for `netmdcli` has been implemented. Some commands weren't visible in the help text,
these have now been added. The JSON track list has been renamed to the `json`
sub-command, and by default, the help message is printed. The CLI `discinfo` command
now shows the text-only disc info (like before the JSON changes).

Right now the commands have mostly been implemented in a backwards-compatible manner,
in the future it might make sense to shuffle around and rename some commands.


## Cleaned out obsolete files in `libnetmd`

The files `CHANGELOG`, `hotplug-netmd` and `minidisc.usermap` were not used/updated
in a long time. They can always be inspected from Git history if needed.


## Progress information callbacks in `libnetmd` for downloading tracks

Added `netmd_send_progress_func` to `libnetmd`'s send-related functions that (if not
NULL) will call back with progress information, so the user interface can show info
about the current progress to the user. This is used by `netmdcli` and QHiMDTransfer.


## Consistent zero-based index for `netmdcli recv`

The `netmdcli recv` function now interprets the track ID as zero-based to be consistent
with other calls that take a zero-based track ID. This change also applies to the
`netmd_secure_recv_track()` function, which previously took a 1-based track ID.


## Utility function `netmd_dev_can_upload()`

Instead of hardcoding string comparisons with "Sony MZ-RH1 (NetMD)", add a utility
function that can query whether or not a NetMD-capable device can upload. Right now,
this is restricted to the MZ-RH1 only, but allows for more semantic features in the future.


## Automatic filename generation in `netmdcli recv`

The `filename` parameter to `netmdcli recv` is now optional. If not supplied, a filename
will be generated based on the track title (or number if the title is empty) and file
type (.aea for SP-mode tracks, .wav for LP2/LP4-mode tracks).


## Progress information callbacks in `libnetmd` for uploading tracks

Similar to the track downloading progress, `libnetmd` now also has support for a
`netmd_recv_progress_func` callback. This is used by `netmdcli` and QHiMDTransfer.

This also removes the duplicated `libnetmd`-based code that was in QHiMDTransfer.


## Add enum types and remove `find_pair()` API for converting values to strings

Add enums `NetMDEncoding`, `NetMDTrackFlags` and `NetMDChannels`. Add functions
`netmd_get_encoding_name()` and `netmd_track_flags_to_string()`. Remove the old
`find_pair()` APIs that were used for this purpose before.


## `netmd_get_track_info()` convenience method and struct

In many cases, a caller wants to retrieve all the information for a track (name,
duration, format) at once, as well as e.g. strip the "LP:" prefix of a track.

This is now possible using a new struct and function to query the track info all
at once. The struct that is filled also has proper enum types, and takes care of
storing the title string, so it's all nice and tidy in a single place.


## Better handling of the `minidisc` struct (groups and disc name)

Added `netmd_minidisc_*()` functions to query disc name, group membership of a
track, group name and whether a group is empty or not. This again combines logic
that existed twice in `netmdcli` and QHiMDTransfer for some time.


## More intelligent "LP:" prefix stripping

MDLP recorders prefix LP2/LP4 recordings with "LP:" in the title, which is shown
on non-MDLP recorders. Instead of stripping a leading "LP:" unconditionally, only
strip it for LP2/LP4 tracks. This allows SP tracks to begin with "LP:" if needed.


## Unified formatting of track and disc time durations

Instead of doing custom formatting in multiple places and accessing the structs,
`netmd_track_duration_to_string()` and `netmd_time_to_string()` are now available
that format a custom structure into a C string. The returned string can be free'd
with `netmd_free_string()`.


## Redesigned `discinfo` output (with ANSI colors) for `netmdcli`

The disc information sub-command of `netmdcli` has been overhauled and now features
fancy ANSI colors (can be disabled with `-n`, and will be automatically disabled
if standard output is not a TTY, e.g. if you pipe the output into `less`).


## Cleaned up CI scripts, Github Actions

The CI scripts now live in `scripts/` instead of `build/`. Travis CI support has
been removed, and replaced with Github Actions. The CI scripts have been simplified.


## Removed `libnetmd_extended.h` header

This was used by QHiMDTransfer's copies of internal functions, but since we have
now moved those back into the library, there's no need to expose those details.


## Moved non-source files out of `libnetmd` into `docs` (or deleted)

The `libnetmd` folder contained lots of non-source files, move those to `docs/`.
Some files were deleted:

 * `libnetmd/documentation/index_files`: No useful content
 * `libnetmd/LICENCE.TXT`: Older version of the GPL2, already exists as `COPYING`
 * Moved `README.vuori` and `README.fedora` to `docs`/
 * `.cdtproject`, `.project`, `Doxyfile`: Deleted


## Whole-project Doxygen tooling

You can run `doxygen` in the project root to get browsable documentation (for the
most part).
