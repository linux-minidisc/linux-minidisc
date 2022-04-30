#pragma once

/** \file descriptor.h */

/*
 * Query formatting/scanning library for libnetmd
 * Based on netmd-js code by asivery (https://github.com/cybercase/netmd-js/pull/29)
 * Copyright (c) 2022, Thomas Perl <m@thp.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "common.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

enum netmd_descriptor {
    NETMD_DESCRIPTOR_DISC_TITLE_TD,
    NETMD_DESCRIPTOR_AUDIO_UTOC1_TD,
    NETMD_DESCRIPTOR_AUDIO_UTOC4_TD,
    NETMD_DESCRIPTOR_DSI_TD,
    NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD,
    NETMD_DESCRIPTOR_ROOT_TD,

    NETMD_DESCRIPTOR_DISC_SUBUNIT_IDENTIFIER,
    NETMD_DESCRIPTOR_OPERATING_STATUS_BLOCK,

    NETMD_DESCRIPTOR_COUNT, /*!< Not a valid descriptor */
};

enum netmd_descriptor_action {
    NETMD_DESCRIPTOR_ACTION_OPEN_READ = 0x01,
    NETMD_DESCRIPTOR_ACTION_OPEN_WRITE = 0x03,
    NETMD_DESCRIPTOR_ACTION_CLOSE = 0x00,
};

/**
 * Change descriptor state.
 *
 * @param dev USB device handle
 * @param descriptor The descriptor to modify
 * @param action Whether to open for reading or writing or to close the descriptor
 */
netmd_error
netmd_change_descriptor_state(netmd_dev_handle *dev,
        enum netmd_descriptor descriptor, enum netmd_descriptor_action action);

#ifdef __cplusplus
}
#endif
