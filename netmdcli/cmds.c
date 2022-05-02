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

#include "cmds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void
netmdcli_print_help(const struct netmdcli_subcommand *cmd)
{
    if (cmd->command == NULL) {
        printf("\n");
        return;
    }

    int len = printf("    %s %s", cmd->name, cmd->argdef);
    while (len < 45) {
        putchar(' ');
        ++len;
    }
    printf("%s\n", cmd->help);
}

void
netmdcli_print_argument_error(struct netmdcli_context *ctx, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "Usage:\n");
    netmdcli_print_help(ctx->cmd);
    fprintf(stderr, "\nERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

static int
netmdcli_context_get_arg_index(struct netmdcli_context *ctx, const char *name)
{
    for (int i=0; i<ctx->argdef_num; ++i) {
        if (strcmp(name, ctx->argdef[i]) == 0) {
            return i;
        }
    }

    return -1;
}

static const char *
netmdcli_context_get_arg(struct netmdcli_context *ctx, const char *name)
{
    int index = netmdcli_context_get_arg_index(ctx, name);

    if (index == -1 || index >= ctx->argc) {
        return NULL;
    }

    return ctx->argv[index];
}

int
netmdcli_context_get_optional_int_arg(struct netmdcli_context *ctx, const char *name)
{
    const char *arg = netmdcli_context_get_arg(ctx, name);

    if (arg == NULL) {
        // Optional argument not supplied
        return -1;
    }

    char *endptr = NULL;

    int result = strtoul(arg, &endptr, 10);
    if (*arg == '\0' || *endptr != '\0') {
        netmdcli_print_argument_error(ctx, "Argument %s is not a valid number: \"%s\"", name, arg);
        exit(2);
    }

    return result;
}

const char *
netmdcli_context_get_optional_string_arg(struct netmdcli_context *ctx, const char *name)
{
    const char *arg = netmdcli_context_get_arg(ctx, name);

    return arg;
}

int
netmdcli_context_get_int_arg(struct netmdcli_context *ctx, const char *name)
{
    int result = netmdcli_context_get_optional_int_arg(ctx, name);
    if (result == -1) {
        netmdcli_print_argument_error(ctx, "Required argument %s was not supplied", name);
        exit(2);
    }

    return result;
}

const char *
netmdcli_context_get_string_arg(struct netmdcli_context *ctx, const char *name)
{
    const char *result = netmdcli_context_get_optional_string_arg(ctx, name);
    if (result == NULL) {
        netmdcli_print_argument_error(ctx, "Required argument %s was not supplied", name);
        exit(2);
    }

    return result;
}

// "32 args ought to be enough for anybody"
#define MAX_ARGS (32)

static int
netmdcli_parse_argdef(const char *argdef, char ***argdef_out, int *argdef_num)
{
    int required_args = 0;

    *argdef_out = malloc(sizeof(char *) * MAX_ARGS);

    char *tmp = strdup(argdef);

    char *cur = tmp;
    char *start = NULL;
    while (*cur) {
        if (*cur == '<' || *cur == '[') {
            if (*cur == '<') {
                ++required_args;
            }

            ++cur;
            start = cur;
        } else if (*cur == '>' || *cur == ']') {
            *cur = '\0';
            (*argdef_out)[(*argdef_num)++] = strdup(start);
            start = NULL;
        }

        ++cur;
    }

    if (start != NULL) {
        fprintf(stderr, "Invalid argdef string: '%s'\n", argdef);
        exit(2);
    }

    free(tmp);

    return required_args;
}

static void
netmdcli_free_argdef(char **argdef, int argdef_num)
{
    for (int i=0; i<argdef_num; ++i) {
        free(argdef[i]);
    }

    free(argdef);
}

static void
netmdcli_free_context(struct netmdcli_context *ctx)
{
    netmdcli_free_argdef(ctx->argdef, ctx->argdef_num);

    free(ctx);
}

static struct netmdcli_context *
netmdcli_create_context(int argc, char *argv[],
        netmd_device *netmd, netmd_dev_handle *devh,
        const struct netmdcli_subcommand *cmd)
{
    struct netmdcli_context *ctx = malloc(sizeof(struct netmdcli_context));

    ctx->netmd = netmd;
    ctx->devh = devh;

    ctx->argc = argc - 2;
    ctx->argv = argv + 2;

    ctx->cmd = cmd;

    ctx->argdef = NULL;
    ctx->argdef_num = 0;

    ctx->required_args = netmdcli_parse_argdef(cmd->argdef, &ctx->argdef, &ctx->argdef_num);

    if (ctx->argc < ctx->required_args) {
        netmdcli_print_argument_error(ctx, "Command %s needs at least %d args (%d given)", cmd->name, ctx->required_args, ctx->argc);

        netmdcli_free_context(ctx);

        return NULL;
    }

    if (ctx->argc > ctx->argdef_num) {
        netmdcli_print_argument_error(ctx, "Command %s takes at most %d args (%d given)", cmd->name, ctx->argdef_num, ctx->argc);

        netmdcli_free_context(ctx);

        return NULL;
    }

    return ctx;
}

enum NetMDCLIHandleResult
netmdcli_handle(const struct netmdcli_subcommand *first_cmd, int argc, char *argv[],
        netmd_device *netmd, netmd_dev_handle *devh)
{
    const struct netmdcli_subcommand *cmd = first_cmd;
    while (cmd->name != NULL) {
        if (cmd->command != NULL && strcmp(cmd->name, argv[1]) == 0) {
            struct netmdcli_context *ctx = netmdcli_create_context(argc, argv, netmd, devh, cmd);

            if (ctx == NULL) {
                return NETMDCLI_ERROR;
            }

            int res = cmd->command(ctx);

            netmdcli_free_context(ctx);

            return (res == 0) ? NETMDCLI_HANDLED : NETMDCLI_ERROR;
        }

        ++cmd;
    }

    return NETMDCLI_UNKNOWN;
}
