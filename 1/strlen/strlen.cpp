﻿#include "strlen.h"

int strLen(const char* str)
{
	int len = 0;

	while (*str++ != '\0')
	{
		len++;
	}

	return len;
}
