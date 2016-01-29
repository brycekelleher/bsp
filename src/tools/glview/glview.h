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
	CMD_FILL,
	CMD_LINES,
	CMD_TRIANGLES,
	CMD_NUM_COMMANDS
};

void Error(const char *error, ...);
void Warning(const char *warning, ...);

// command buffer
void BufferInit(int numbytes);
void BufferWriteBytes(void *data, int numbytes);
void BufferReadBytes(unsigned int addr, void *data, int numbytes);
void BufferFlush();
void BufferCommit();
int BufferWriteAddr();
int BufferCommitAddr();

// file parsing
void Read(FILE *fp);

// rendering
void DrawInit(int argc, char *argv[]);
int DrawMain(int argc, char *argv[]);

#endif
