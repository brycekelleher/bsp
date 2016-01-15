#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include "glview.h"

// ==============================================
// errors and warnings

void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);

	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "Error: %s", buffer);
	fprintf(stderr, "\x1b[0m");
	exit(1);
}

void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);

	fprintf(stdout, "\x1b[33m");
	fprintf(stdout, "Warning: %s", buffer);
	fprintf(stdout, "\x1b[0m");
}

static void ProcessEnviromentVariables(int argc, char *argv[])
{
	char *buffersizestr = getenv("GLVIEW_BUFFER_SIZE");
}

static void ProcessCommandLine(int argc, char *argv[])
{}

static void PrintUsage()
{}

#if 0
static void ReadBinaryImage(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF)
	{
		printf("%x\n", c);
		BufferWriteBytes(&c, 1);
	}
	BufferCommit();
	printf("committed buffer\n");
}
#endif

#if 0
// this is for stdin which may not be eof terminated (eg driven by netcat or fifo)
static void ReadBinaryImageStream(FILE *fp)
{
	int c;

	while ((c = fgetc(fp)) != EOF)
	{
		printf("%x\n", c);
		BufferWriteBytes(&c, 1);
		BufferCommit();
	}
	printf("got eof\n");
}
#endif

static int gargc;
static char **gargv;

static void FifoReadLoop()
{
	while(1)
	{
		static const char *fifoname = "/tmp/f";
		FILE *fp = fopen(fifoname, "rb");
		if(!fp)
			Error("couldn't open fifo \"%s\"", fifoname);

		Read(fp);
	}
}

static void *FrontEndThread(void *args)
{
	int argc = gargc;
	char **argv = gargv;

	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++)
	{
		if (!strcmp(argv[i], "--fifo"))
		{
			FifoReadLoop();
			return EXIT_SUCCESS;
		}
	}

	if (i == argc)
	{
		Read(stdin);
	}
	else
	{
		for (; i < argc; i++)
		{
			char *filename = argv[i];
			FILE *fp = fopen(filename, "rb");
			if(!fp)
				Error("couldn't open file \"%s\"", filename);
			
			Read(fp);
		}
	}

	return EXIT_SUCCESS;
}

static void *DrawThread(void *args)
{
	int argc = gargc;
	char **argv = gargv;

	DrawMain(argc, argv);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	gargc = argc;
	gargv = argv;

	BufferInit(32 * 1024 * 1024);

	BufferFlush();

	pthread_t front, draw;
	pthread_create(&front, NULL, FrontEndThread, NULL);
	pthread_create(&draw, NULL, DrawThread, NULL);

	pthread_join(front, NULL);
	pthread_join(draw, NULL);

	return 0;
}

