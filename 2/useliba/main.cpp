#include <stdio.h>

#include "strlen.h"

/*
��̬�����ӣ�
�������libstr.a�⣬ʹ��ͷ�ļ�����

��������a��̬��д��outĿ���ļ�������ʱ����Ҫ����a��̬��
*/

int main(void)
{
	const char* pStr = "Hello, World!";

	int len = strLen(pStr);

	printf("len = %i\n", len);

	return 0;
}
