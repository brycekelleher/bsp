#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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

//============================


int ReadBytes(void *buf, int numbytes, FILE *fp)
{
	return fread(buf, sizeof(unsigned char), numbytes, fp);
}

int ReadInt(FILE *fp)
{
	int i;
	
	ReadBytes(&i, sizeof(int), fp);
	return i;
}

float ReadFloat(FILE *fp)
{
	float f;

	ReadBytes(&f, sizeof(float), fp);
	return f;
}

static void DecodePolygon(FILE *fp)
{
	int numvertices = ReadInt(fp);

	for (int i = 0; i < numvertices; i++)
	{
		float x, y, z;
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);

		printf("vertex %i: %f %f %f\n", x, y, z);
	}
}

static void DecodePortals(FILE *fp)
{
	int numportals = ReadInt(fp);

	for (int i = 0; i < numportals; i++)
		DecodePolygon(fp);
}

static void DecodeNode(int nodenum, FILE *fp)
{
	if(nodenum == -1)
	{
		return;
	}
	
	printf("node %i:\n", nodenum);
	
	int childnum[2];
	childnum[0] = ReadInt(fp);
	childnum[1] = ReadInt(fp);
	printf("children: %i, %i\n", childnum[0], childnum[1]);

	float a, b, c, d;
	a = ReadFloat(fp);
	b = ReadFloat(fp);
	c = ReadFloat(fp);
	d = ReadFloat(fp);
	printf("plane: %f, %f, %f, %f\n", a, b, c, d);

	float min[3], max[3];
	min[0] = ReadFloat(fp);
	min[1] = ReadFloat(fp);
	min[2] = ReadFloat(fp);
	max[0] = ReadFloat(fp);
	max[1] = ReadFloat(fp);
	max[2] = ReadFloat(fp);
	printf("boxmin: %f, %f, %f\n", min[0], min[1], min[2]);
	printf("boxmax: %f, %f, %f\n", max[0], max[1], max[2]);

	DecodePortals(fp);

	DecodeNode(childnum[0], fp);
	DecodeNode(childnum[1], fp);
}

static void DecodeTree(FILE *fp)
{
	printf("tree:\n");
	
	int numnodes = ReadInt(fp);
	printf("numnodes: %i\n", numnodes);

	int numleafs = ReadInt(fp);
	printf("numleafs: %i\n", numleafs);
	
	DecodeNode(0, fp);
}

static void DecodeTriSurface(FILE *fp)
{
	int numvertices = ReadInt(fp);

	for (int i = 0; i < numvertices; i++)
	{
		float x, y, z;
		x = ReadFloat(fp);
		y = ReadFloat(fp);
		z = ReadFloat(fp);

		printf("vertex %i: %f %f %f\n", x, y, z);
	}
}

static void DecodeAreaSurfaces(FILE *fp)
{
	DecodeTriSurface();
}

static void DecodeFile(FILE *fp)
{
	DecodeTree(fp);

	DecodeAreaSurfaces(fp);
}

// fixme: move this into another file
static FILE *OpenFile(const char *filename)
{
	FILE *fp;
	
	fp = fopen(filename, "rb");
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);
	
	return fp;
}

static void PrintUsage()
{
	printf(
	"<options> <inputfiles ...>\n"
	"-o <filename>		output filename\n"
	);

	exit(0);
}

static void ProcessEnvVars()
{}

static void ProcessCommandLine(int argc, char *argv[])
{
	int i;
	
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	for(i = 1; argv[i][0] == '-'; i++)
	{
		Error("Unknown option \"%s\"\n", argv[i]);
	}
	
	{
		FILE *fp = OpenFile(argv[i]);
		DecodeFile(fp);
	}
}


int main(int argc, char *argv[])
{
	ProcessCommandLine(argc, argv);
}
