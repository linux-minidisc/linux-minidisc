import re
import collections
import json

current_mode = None

models_by_mode = collections.defaultdict(list)

with open('minidisc.devids') as fp:
    for line in fp:
        # remove any comments
        if '#' in line:
            line = line[:line.find('#')]

        # remove trailing whitespace
        line = line.rstrip()

        if not line:
            continue

        # sections, e.g. [netmd] and [himd]
        m = re.match(r'^\[(.*)\]$', line)
        if m is not None:
            current_mode = m.group(1)
            continue

        # devices, e.g. vvid ppid "Name"
        m = re.match(r'^([0-9a-fA-F]{4}) ([0-9a-fA-F]{4}) "(.*)"$', line)
        if m is not None:
            vid, pid, name = m.groups()

            vid = int(vid, 16)
            pid = int(pid, 16)

            assert current_mode is not None
            models_by_mode[current_mode].append((vid, pid, name))

            continue

        raise ValueError(line)

# Sort each device list in-place
for mode, devices in models_by_mode.items():
    devices.sort()

    # Make sure we don't have two entries for the same (vid, pid) pair
    vids_pids = [(vid, pid) for vid, pid, name in devices]
    assert len(set(vids_pids)) == len(vids_pids), f'Duplicate entries for {mode}'

# Export netmd-js style device list
with open('netmd-js/netmd.devices.ts', 'w') as fp:
    print('export const DevicesIds = [', file=fp)
    for vid, pid, name in models_by_mode['netmd']:
        print(f"    {{ vendorId: 0x{vid:04x}, deviceId: 0x{pid:04x}, name: '{name}' }},", file=fp)
    print('];', file=fp)

# Export udev rules
with open('../qhimdtransfer/linux/etc/udev/60-minidisc.rules', 'w') as fp:
    print('# NetMD and Hi-MD devices', file=fp)
    print('# Generated from minidisc.devids', file=fp)
    print('', file=fp)

    for mode, devices in sorted(models_by_mode.items()):
        print(f'## {mode.upper()}', file=fp)
        print('', file=fp)
        for vid, pid, name in devices:
            print(f'# {name}', file=fp)
            print(f'ATTRS{{idVendor}}=="{vid:04x}", ATTRS{{idProduct}}=="{pid:04x}", MODE="0664", GROUP="plugdev"', file=fp)
            print('', file=fp)

# Export for libnetmd
with open('../libnetmd/netmd_dev_ids.h', 'w') as fp:
    print('#include "netmd_dev.h"', file=fp)
    print('#include <stddef.h>', file=fp)
    print('', file=fp)
    print('static struct netmd_devices const known_devices[] = {', file=fp)
    for vid, pid, name in models_by_mode['netmd']:
        print(f'    {{ 0x{vid:04x}, 0x{pid:04x}, "{name}" }},', file=fp)
    print(f"    {{ 0, 0, NULL }}, /* sentinel */", file=fp)
    print('};', file=fp)

# Export for netmd
with open('../netmd/libnetmd_dev_ids.py', 'w') as fp:
    print('KNOWN_USB_ID_SET = frozenset([', file=fp)
    for vid, pid, name in models_by_mode['netmd']:
        print(f'    (0x{vid:04x}, 0x{pid:04x}), # {name}', file=fp)
    print('])', file=fp)

# Helper for QHiMDTransfer
with open('../qhimdtransfer/qhimddetection_dev_ids.h', 'w') as fp:
    print('#pragma once', file=fp)
    print('const char *identify_usb_device(int vid, int pid);', file=fp)

with open('../qhimdtransfer/qhimddetection_dev_ids.cpp', 'w') as fp:
    print('#include "qhimddetection_dev_ids.h"', file=fp)
    print('#include <cstddef>', file=fp)
    print('', file=fp)
    print('const char *identify_usb_device(int vid, int pid) {', file=fp)
    for mode, devices in sorted(models_by_mode.items()):
        for vid, pid, name in devices:
            print(f'    if (vid == 0x{vid:04x} && pid == 0x{pid:04x}) {{', file=fp)
            if mode == 'netmd':
                print(f'        return "{name} (NetMD)";', file=fp)
            else:
                print(f'        return "{name}";', file=fp)
            print('    }', file=fp)
            print('', file=fp)
    print('    return NULL;', file=fp)
    print('}', file=fp)
