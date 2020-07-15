#include <stdio.h>

#include "strlen.h"

/*
静态库链接：
工程添加libstr.a库，使用头文件引用

链接器将a静态库写入out目标文件，发布时不需要带上a静态库
*/

int main(void)
{
	const char* pStr = "Hello, World!";

	int len = strLen(pStr);

	printf("len = %i\n", len);

	return 0;
}
