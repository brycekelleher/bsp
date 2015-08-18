#ifndef __GLDRAW_H__
#define __GLDRAW_H__

#include <stdlib.h>
#include <stdio.h>

enum cmdtype_t
{
	CMD_FLUSH,
	CMD_END,
	CMD_COLOR,
	CMD_CULL,
	CMD_LINES,
	CMD_TRIANGLES,
	CMD_NUM_COMMANDS
};

// don't think this is used...
struct cmd_t
{
	int		size;
	cmdtype_t	type;
};

void Error(const char *error, ...);
void Warning(const char *warning, ...);

// command buffer
void BufferInit();
void BufferWriteBytes(void *data, int numbytes);
void BufferReadBytes(void *data, int numbytes);
void BufferFlush();
void BufferRewind();

// file parsing
void Read(FILE *fp);

// rendering
void DrawInit(int argc, char *argv[]);
int DrawMain();

#endif
