#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	close(1);
	open("redirect.txt", O_WRONLY | O_TRUNC | O_CREAT | O_EXCL);
	printf("Hello World.\n");

	return 0;
}