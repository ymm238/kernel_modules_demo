#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE		4096


int main()
{
	void *buffer;

	// 1GB
	for (int i = 0; i < 262144; i++) {
		buffer = NULL;
		buffer = malloc(BUFFER_SIZE);
		if (buffer)
			memset(buffer, 0, BUFFER_SIZE);
		else
			printf("malloc failed.");
	}

	sleep(1000);
	return 0;
}
