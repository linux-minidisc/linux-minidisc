/* netmd_trace.c
 *      Copyright (C) 2004, Bertrik Sikken
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

/*	Very simple tracing facility.
*/


#include <stdio.h>
#include <stdarg.h>
  
#include "libnetmd.h"
 

static int trace_level = 0;

 
void netmd_trace_level(int level)
{
 	trace_level = level;
}


void netmd_trace_hex(int level, unsigned char *buf, int len)
{
 	int i;
	int j = 0;
	int breakpoint = 0;

	if (level > trace_level) {
		return;
	}
	
	for (i = 0; i < len; i++)
	{
		printf("%02x ", buf[i]);
		breakpoint++;
		if(!((i + 1)%16) && i)
		{
			printf("\t\t");
			for(j = ((i+1) - 16); j < ((i+1)/16) * 16; j++)
			{
				if(buf[j] < 30)
					printf(".");
				else
					printf("%c", buf[j]);
			}
			printf("\n");
			breakpoint = 0;
		}
	}

	if(breakpoint == 16)
	{
		printf("\n");
		return;
	}

	for(; breakpoint < 16; breakpoint++)
	{
		printf("   ");
	}
	printf("\t\t");

	for(j = len - (len%16); j < len; j++)
	{
		if(buf[j] < 30)
			printf(".");
		else
			printf("%c", buf[j]);
	}
	printf("\n");
}


void netmd_trace(int level, char *fmt, ...)
{
	va_list	arg;
	
	if (level > trace_level) {
		return;
	}
	
	va_start(arg, fmt);
	vprintf(fmt, arg);
	va_end(arg);
}


