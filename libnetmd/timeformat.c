/*
 * timeformat.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "libnetmd.h"

#include <stdio.h>
#include <stdlib.h>

static const size_t
TIME_STRING_MAX_LEN = 16;

char *netmd_track_duration_to_string(const struct netmd_track *duration)
{
    char *result = malloc(TIME_STRING_MAX_LEN);
    snprintf(result, TIME_STRING_MAX_LEN, "%02i:%02i:%02i", duration->minute, duration->second, duration->tenth);
    return result;
}

char *netmd_time_to_string(const netmd_time *time)
{
    char *result = malloc(TIME_STRING_MAX_LEN);
    snprintf(result, TIME_STRING_MAX_LEN, "%02d:%02d:%02d.%02d", time->hour, time->minute, time->second, time->frame);
    return result;
}

void netmd_free_string(char *string)
{
    free(string);
}
