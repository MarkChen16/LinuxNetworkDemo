#include <stdio.h>

#include "strlen.h"

/*
共享库链接：
工程加入libstrlen.so，使用头文件引用

链接器将so写入out目标文件，发布不需要带so文件
*/

int main(void)
{
	const char* pStr = "Hello, World!";

	int len = strLen(pStr);

	printf("len = %i\n", len);

	return 0;
}
