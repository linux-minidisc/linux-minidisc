#pragma once

/* netmdcli command line handling
 * Copyright (C) 2022, Thomas Perl <m@thp.io>
 *
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <unistd.h>

struct netmdcli_subcommand;

#include "libnetmd.h"
#include "netmd_dev.h"

struct netmdcli_context {
    netmd_device *netmd;
    netmd_dev_handle *devh;

    // passed-in command line arguments
    int argc;
    char **argv;

    // the currently-executing subcommand
    const struct netmdcli_subcommand *cmd;

    // this command's argument definition
    char **argdef;
    int argdef_num;
    int required_args;
};

struct netmdcli_subcommand {
    const char *name;
    int (*command)(struct netmdcli_context *ctx);
    const char *argdef;
    const char *help;
};

void
netmdcli_print_help(const struct netmdcli_subcommand *cmd);

void
netmdcli_print_argument_error(struct netmdcli_context *ctx, const char *fmt, ...);

int
netmdcli_context_get_optional_int_arg(struct netmdcli_context *ctx, const char *name);

const char *
netmdcli_context_get_optional_string_arg(struct netmdcli_context *ctx, const char *name);

int
netmdcli_context_get_int_arg(struct netmdcli_context *ctx, const char *name);

const char *
netmdcli_context_get_string_arg(struct netmdcli_context *ctx, const char *name);


enum NetMDCLIHandleResult {
    NETMDCLI_HANDLED = 0,
    NETMDCLI_UNKNOWN = 1,
    NETMDCLI_ERROR = 2,
};

enum NetMDCLIHandleResult
netmdcli_handle(const struct netmdcli_subcommand *first_cmd, int argc, char *argv[],
        netmd_device *netmd, netmd_dev_handle *devh);
