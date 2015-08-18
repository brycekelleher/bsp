#include <stdlib.h>
#include "glview.h"

struct buffer_t
{
	unsigned char *data;
	int readpos;
	int writepos;
};

buffer_t buffer;

//
// command buffer code
//
void BufferInit()
{
	buffer.data = (unsigned char*)malloc(32 * 1024 * 1024);
	buffer.readpos = 0;
	buffer.writepos = 0;
}

void BufferWriteBytes(void *data, int numbytes)
{
	unsigned char *src = (unsigned char*)data;
	unsigned char *dst = (unsigned char*)(buffer.data + buffer.writepos);

	for(int i = 0; i < numbytes; i++)
	{
		dst[i] = src[i];
	}

	buffer.writepos += numbytes;
}

void BufferReadBytes(void *data, int numbytes)
{
	unsigned char *src = (unsigned char*)(buffer.data + buffer.readpos);
	unsigned char *dst = (unsigned char*)data;

	for(int i = 0; i < numbytes; i++)
		dst[i] = src[i];

	buffer.readpos += numbytes;
}

void BufferFlush()
{
	// reset the buffer write position to the start
	buffer.writepos = 0;
}

void BufferRewind()
{
	// reset the buffer read position to the start
	buffer.readpos = 0;
}

