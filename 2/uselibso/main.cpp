#include <stdio.h>

#include "strlen.h"

/*
��������ӣ�
���̼���libstrlen.so��ʹ��ͷ�ļ�����

��������soд��outĿ���ļ�����������Ҫ��so�ļ�
*/

int main(void)
{
	const char* pStr = "Hello, World!";

	int len = strLen(pStr);

	printf("len = %i\n", len);

	return 0;
}
