#include <stdio.h>

//引用dlfcn.h  链接libdl.a
#include <dlfcn.h>


/*
方法一：全局共享库：复制.so到/lib，执行ldconfig加载到缓存
方法二：添加工程目录到搜索目录列表 export LD_LIBRARY_PATH=/example/ex02: $LD_LIBRARY_PATH
方法三：
1、临时修改，log out之后就失效
在terminal中执行：export LD_LIBRARY_PATH=./: $LD_LIBRARY_PATH

2、让当前帐号以后都优先加载当前目录的动态库
修改~/.bash_profile在文件末尾加上两行： LD_LIBRARY_PATH=./ 和 export LD_LIBRARY_PATH

3、让所有帐号从此都优先加载当前目录的动态库
修改/etc/profile在文件末尾加上两行： LD_LIBRARY_PATH=./ 和 export LD_LIBRARY_PATH
*/

/*
共享库动态链接：
引用dlfcn.h头文件，添加dl库依赖项
动态加载发布时需要带上so文件

打开so动态库
void* dlopen(const char* fileName, int mode)

获取函数指针
void* dlsym(void* handle, const char* symName)

获取错误信息
char* dlerror()

关闭so动态库
int dlclose(void* handle)

*/

typedef int (*funStrLen)(const char* str);

int main(void)
{
	//动态加载so
	void* handle = NULL;
	char* pErr = NULL;

	handle = dlopen("./libstrlen.so", RTLD_LAZY);
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
