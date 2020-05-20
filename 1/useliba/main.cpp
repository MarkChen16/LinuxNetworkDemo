#include <stdio.h>

#include "strlen.h"

int main(void)
{
	const char* pStr = "Hello, World!";

	int len = strLen(pStr);

	printf("len = %i\n", len);

	return 0;
}
