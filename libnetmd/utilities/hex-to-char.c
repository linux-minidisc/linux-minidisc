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
