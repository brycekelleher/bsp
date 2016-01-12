#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
{}

static void ProcessCommandLine(int argc, char *argv[])
{}

static void PrintUsage()
{}

static void ReadImage(FILE *fp)
{
	int c;

	BufferFlush();
	while ((c = fgetc(fp)) != EOF)
		BufferWriteBytes(&c, 1);
	BufferCommit();
}

//
// Main
//
int main(int argc, char *argv[])
{
	BufferInit();

	for(int i = 1; i < argc; i++)
	{
		char *filename = argv[i];

		FILE *fp = fopen(filename, "rb");
		if(!fp)
		{
			printf("couldn't open file \"%s\"", filename);
			continue;
		}
		
		ReadImage(fp);
	}

	DrawMain(argc, argv);

	return 0;
}

