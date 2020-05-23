#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int ret = 0;

	if (argc != 3)
	{
		printf("Error; argc != 3\n");
		ret = 1;
	}
	else
	{
		const char* filePath = argv[1];
		const char* textData = argv[2];
		int fd = -1;

		fd = open(filePath, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd == -1)
		{
			printf("Failed to open %s\n", filePath);
			ret = 2;
		}
		else
		{
			printf("Success to open %s, fd=%d\n", filePath, fd);

			ssize_t writeLen = write(fd, textData, strlen(textData));
			if (writeLen == -1)
			{
				printf("Failed to write file.\n");
				ret = 3;
			}
			else
			{
				printf("Write %d bytes to %s\n", writeLen, filePath);
				writeLen = write(fd, "\n", 1);
			}

			close(fd);
		}
	}

	return 0;
}
