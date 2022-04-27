/* hex-to-char.c 
 *      Copyright (C) 2002, 2003 Marc Britten
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
#include <stdio.h>

int main(int argc, char* argv[])
{
	int i = 0;
	int j = 0;
	int iLen = 0;
	int iData = 0;
	char hex[5];

	if(argc < 2)
		return 0;

	for(j = 1; j < argc; j++)
	{
		sprintf(hex, "0x%s", argv[j]);
		iData = strtol(hex, NULL, 16);

		if(iData < 30)
			printf(". ");
		else
			printf("%c ", iData);
	}
	
	printf("\n");
	return 0;
}
