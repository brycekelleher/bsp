#include <stdlib.h>
#include "glview.h"

struct buffer_t
{
	unsigned char *data;
	int wpos;
	int cpos;
};

buffer_t buffer;

void BufferFlush()
{
	buffer.wpos = buffer.cpos = 0;
}

void BufferCommit()
{
	buffer.cpos = buffer.wpos;
}

int BufferWriteAddr()
{
	return buffer.wpos;
}

int BufferCommitAddr()
{
	return buffer.cpos;
}

void BufferWriteBytes(void *data, int numbytes)
{
	unsigned char *src = (unsigned char*)data;
	unsigned char *dst = (unsigned char*)(buffer.data + buffer.wpos);

	for (int i = 0; i < numbytes; i++)
		dst[i] = src[i];

	buffer.wpos += numbytes;
}

// fixme: assert that the address is valid
void BufferReadBytes(unsigned int addr, void *data, int numbytes)
{
	unsigned char *src = (unsigned char*)(buffer.data + addr);
	unsigned char *dst = (unsigned char*)data;

	for (int i = 0; i < numbytes; i++)
		dst[i] = src[i];
}

void BufferInit(int numbytes)
{
	buffer.data = (unsigned char*)malloc(numbytes);

	BufferFlush();
}


