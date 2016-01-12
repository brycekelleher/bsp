#include <stdlib.h>
#include "glview.h"

struct buffer_t
{
	unsigned char *data;
	int wpos;
	int cpos;
};

buffer_t buffer;

//
// command buffer code
//
void BufferInit()
{
	buffer.data = (unsigned char*)malloc(32 * 1024 * 1024);
	buffer.wpos = buffer.cpos = 0;
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

void BufferFlush()
{
	buffer.wpos = 0;
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

