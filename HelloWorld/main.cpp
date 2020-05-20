#include <stdio.h>

int main(void)
{
	printf("Hello, world!\n");

	//中文会引起乱码，linux使用utf8编码，但windows使用的是GBK，要把全部源代码修改为Unicode无签名，unix行尾；
	printf("你好，世界！\n");

	return 0;
}
