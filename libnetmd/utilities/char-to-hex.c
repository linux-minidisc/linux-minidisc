/* char-to-hex.c 
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

	if(argc < 2)
		return 0;


	for(j = 1; j < argc; j++)
	{
		iLen = strlen(argv[j]);
		for(i = 0; i < iLen; i++)
			{
				printf(" %c ", argv[j][i]);
			}

		printf("\n");	
		for(i = 0; i < iLen; i++)
			{
				printf("%02x ", argv[j][i]);
			}
	
		printf("\n");
	}
			return 0;
		}
