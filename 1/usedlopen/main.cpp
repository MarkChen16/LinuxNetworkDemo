#include <stdio.h>

//引用dlfcn.h  链接libdl.a
#include <dlfcn.h>

//方法一：全局共享库：复制.so到/lib，执行ldconfig加载到缓存
//方法二：添加工程目录到搜索目录列表 export LD_LIBRARY_PATH=/example/ex02: $LD_LIBRARY_PATH

typedef int (*funStrLen)(const char* str);

int main(void)
{
	//动态加载so
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
		//使用readelf -s libstrlen.so 读取符号表，查找strLen的C++函数名称
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
				//调用接口
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
