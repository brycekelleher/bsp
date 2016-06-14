#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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
	
	fprintf(stderr, "\x1b[33m");
	fprintf(stderr, "Warning: %s", buffer);
	fprintf(stderr, "\x1b[0m");
}

// ==============================================
// Files

FILE *FileOpen(const char *filename, const char* mode)
{
	FILE *fp = fopen(filename, mode);
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);
	
	return fp;
}

FILE *FileOpenBinaryRead(const char *filename)
{
	return FileOpen(filename, "rb");
}

FILE *FileOpenBinaryWrite(const char *filename)
{
	return FileOpen(filename, "wb");
}

FILE *FileOpenTextRead(const char *filename)
{
	return FileOpen(filename, "r");
}

FILE *FileOpenTextWrite(const char *filename)
{
	return FileOpen(filename, "w");
}

bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		return false;
	
	fclose(fp);
	return true;
}

int FileSize(FILE *fp)
{
	int curpos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, curpos, SEEK_SET);
	
	return size;
}

void FileClose(FILE *fp)
{
	fclose(fp);
}

void WriteFile(const char *filename, void *data, int numbytes)
{
	FILE *fp = fopen(filename, "wb");
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);

	fwrite(data, numbytes, 1, fp);
	fclose(fp);
}

int ReadFile(const char* filename, void **data)
{
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);

	int size = FileSize(fp);
	
	*data = malloc(size);
	fread(*data, size, 1, fp);
	fclose(fp);
	
	return size;
}

void WriteBytes(void *data, int numbytes, FILE *fp)
{
	fwrite(data, numbytes, 1, fp);
}

int ReadBytes(void *buf, int numbytes, FILE *fp)
{
	return fread(buf, sizeof(unsigned char), numbytes, fp);
}

