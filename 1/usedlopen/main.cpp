#include <stdio.h>

//����dlfcn.h  ����libdl.a
#include <dlfcn.h>

//����һ��ȫ�ֹ���⣺����.so��/lib��ִ��ldconfig���ص�����
//����������ӹ���Ŀ¼������Ŀ¼�б� export LD_LIBRARY_PATH=/example/ex02: $LD_LIBRARY_PATH

typedef int (*funStrLen)(const char* str);

int main(void)
{
	//��̬����so
	void* handle = NULL;
	char* pErr = NULL;

	handle = dlopen("libstrlen.so", RTLD_LAZY);
	if (!handle)
	{
		printf("Failed load library!\n");
		return 0;
	}

	pErr = dlerror();
	if (pErr)
	{
		printf("Err: %s\n", pErr);
		return 0;
	}
	else
	{
		//ʹ��readelf -s libstrlen.so ��ȡ���ű�����strLen��C++��������
		funStrLen pfStrLen = (funStrLen)dlsym(handle, "_Z6strLenPKc");
		if (!pfStrLen)
		{
			printf("Failed get symbool!\n");
		}
		else
		{
			pErr = dlerror();
			if (pErr)
			{
				printf("Err: %s\n", pErr);
				return 0;
			}
			else
			{
				//���ýӿ�
				const char* pStr = "Hello, World!";

				int len = pfStrLen(pStr);

				printf("len = %i\n", len);
			}
		}
	}

	dlclose(handle);
	handle = NULL;

	return 0;
}
