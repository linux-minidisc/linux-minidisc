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
