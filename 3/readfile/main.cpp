#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//使用root账户运行ln -s /xx/xx/../readfile.out /bin/xread   将程序变成一个小工具
//使用例子：xread data.txt

//main参数：1、当前程序路径，2、参数...

int main(int argc, char* argv[])
{
	int ret = 0;

	if (argc < 2)
	{
		printf("No file path.\n");
		ret = 1;
	}
	else
	{
		const char* filePath = argv[1];
		int fd = -1;
		
		fd = open(filePath, O_RDONLY);
		if (fd == -1)
		{
			printf("Failed to open [%s]\n", filePath);
			ret = 2;
		}
		else
		{
			printf("File[% s]: \n", filePath);

			const int buffSize = 100;
			char buff[buffSize];
			ssize_t readCount = -1;

			do 
			{
				readCount = read(fd, buff, buffSize);
				if (readCount == -1)
				{
					printf("Failed to read [%s]\n", filePath);
					ret = 3;
				}
				else if (readCount > 0)
				{
					printf("read %d bytes: ", readCount);

					for (int i = 0; i < readCount; i++)
					{
						if (buff[i] != ' ' && buff[i] != '\t' && buff[i] != '\x0a' && buff[i] != '\x0d')
							printf("%c", buff[i]);
					}

					printf("\n");
				}

			} while (readCount > 0);

			printf("[Finished]\n");
			close(fd);
		}
	}

	return ret;
}
