#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gldraw.h"

static void EmitCommand(cmdtype_t cmdtype)
{
	BufferWriteBytes(&cmdtype, 4);
}

static void EmitFloat(float f)
{
	BufferWriteBytes(&f, 4);
}

static void EmitInt(int i)
{
	BufferWriteBytes(&i, 4);
}

static void EmitUnsignedInt(unsigned int ui)
{
	BufferWriteBytes(&ui, 4);
}

#if 0
// fixme: replace with this?
static char *ReadToken(FILE *fp)
{
	int c;

	c = fgetc(fp);
	for (; c != EOF && isspace(c); c = fgetc(fp))
		if (c == '\n')
			linenum++;

	int i = 0;
	for (; c != EOF && !isspace(c); c = fgetc(fp))
		buffer[i++] = c;

	buffer[i] = '\0';

	if (i == 0)
		return NULL;

	return buffer;
}
#endif

// returns a pointer to a static
static char* ReadToken(FILE *fp)
{
	static char buffer[1024];

	if (feof(fp))
		return NULL;

	fscanf(fp, "%s", buffer);
	return buffer;
}

static void Eatline(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF)
		if (c == '\n')
			return;
}

static float ReadFloat(FILE *fp)
{
	char *token = ReadToken(fp);
	return atof(token);
}

static int ReadInt(FILE *fp)
{
	char *token = ReadToken(fp);
	return atoi(token);
}

static void ReadColor(FILE *fp)
{
	float r, g, b, a;

	r = ReadFloat(fp);
	g = ReadFloat(fp);
	b = ReadFloat(fp);
	a = ReadFloat(fp);

	EmitCommand(CMD_COLOR);
	EmitFloat(r);
	EmitFloat(g);
	EmitFloat(b);
	EmitFloat(a);
}

static void ReadCull(FILE *fp)
{
	EmitCommand(CMD_CULL);
	EmitInt(ReadInt(fp));
}

static void ReadLine(FILE *fp)
{
	float x, y, z;

	EmitCommand(CMD_LINES);
	EmitInt(2);

	x = ReadFloat(fp);
	y = ReadFloat(fp);
	z = ReadFloat(fp);
	EmitFloat(x);
	EmitFloat(y);
	EmitFloat(z);

	x = ReadFloat(fp);
	y = ReadFloat(fp);
	z = ReadFloat(fp);
	EmitFloat(x);
	EmitFloat(y);
	EmitFloat(z);
}

static void ReadLineList(FILE *fp)
{
	int numlines, numvertices;

	EmitCommand(CMD_LINES);

	numlines = ReadInt(fp);
	numvertices = numlines * 2;
	EmitInt(numvertices);

	while (numvertices)
	{
		float x, y, z;

		// read and emit the next vertex
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);

		numvertices--;
	}
}

static void ReadTriangle(FILE *fp)
{
	EmitCommand(CMD_TRIANGLES);
	EmitInt(3);

	for(int i = 0; i < 3; i++)
	{
		float x, y, z;

		// read and emit the next vertex
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);
	}
}

static void ReadTriangleList(FILE *fp)
{
	int numtris, numvertices;

	EmitCommand(CMD_TRIANGLES);

	numtris = ReadInt(fp);
	numvertices = numtris * 3;
	EmitInt(numvertices);

	while (numvertices)
	{
		float x, y, z;

		// read and emit the next vertex
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);

		numvertices--;
	}
}

static void ReadPolyline(FILE *fp)
{
	int numvertices, numlines;
	float x, y, z;
	
	EmitCommand(CMD_LINES);

	// read numvertices
	numvertices = ReadInt(fp);
	numlines = numvertices - 1;
	EmitInt(numlines * 2);

	// read the first vertex
	x = ReadFloat(fp);
	y = ReadFloat(fp);
	z = ReadFloat(fp);

	// read the vertex data
	for(int i = 0; i < numlines; i++)
	{
		// emit the previous vertex
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);

		// read and emit the next vertex
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);
	}
}

static void ReadPolygon(FILE *fp)
{
	int numvertices, numtris;
	float x0, y0, z0;
	float x, y, z;
	
	EmitCommand(CMD_TRIANGLES);

	// read numvertices
	numvertices = ReadInt(fp);
	numtris = numvertices - 2;
	EmitInt(numtris * 3);

	// read the first vertex
	x0 = ReadFloat(fp);
	y0 = ReadFloat(fp);
	z0 = ReadFloat(fp);

	// read the second vertex
	x = ReadFloat(fp);
	y = ReadFloat(fp);
	z = ReadFloat(fp);

	for(int i = 0; i < numtris; i++)
	{
		// emit the pivot vertex
		EmitFloat(x0);
		EmitFloat(y0);
		EmitFloat(z0);

		// emit the previous vertex
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);

		// read and emit the next vertex
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);
		EmitFloat(x);
		EmitFloat(y);
		EmitFloat(z);
	}
}

//
// read entry point
//
void Read(FILE *fp)
{
	char *token;

	while ((token = ReadToken(fp)))
	{
		// eat comments
		if (token[0] == '/' && token[1] == '/')
		{
			Eatline(fp);
			continue;
		}

		if (!strcmp("color", token))
			ReadColor(fp);
		else if (!strcmp("cull", token))
			ReadCull(fp);
		else if (!strcmp("line", token))
			ReadLine(fp);
		else if (!strcmp("linelist", token))
			ReadLineList(fp);
		else if (!strcmp("lines", token))
			ReadLineList(fp);
		else if (!strcmp("triangle", token))
			ReadTriangle(fp);
		else if (!strcmp("trianglelist", token))
			ReadTriangleList(fp);
		else if (!strcmp("triangles", token))
			ReadTriangleList(fp);
		else if (!strcmp("polyline", token))
			ReadPolyline(fp);
		else if (!strcmp("polygon", token))
			ReadPolygon(fp);
	}

	// terminate the command buffer
	EmitCommand(CMD_END);
}

