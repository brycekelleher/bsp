#include "bsp.h"

const char	*outputfilename = "out.bsp";
static bool	verbose = false;

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

void Message(const char *format, ...)
{
	if(!verbose)
	{
		return;
	}
	
	va_list valist;
	char buffer[2048];
	
	va_start(valist, format);
	vsprintf(buffer, format, valist);
	va_end(valist);

	fprintf(stdout, "%s", buffer);
}

// ==============================================
// Memory allocation

void *Malloc(int numbytes)
{
	void *mem;
	
	mem = malloc(numbytes);

	if(!mem)
	{
		Error("Malloc: Failed to allocated memory");
	}

	return mem;
}

void *MallocZeroed(int numbytes)
{
	void *mem;
	
	mem = Malloc(numbytes);

	memset(mem, 0, numbytes);

	return mem;
}

// ==============================================
// Main

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
		if(!strcmp(argv[i], "-o"))
		{
			i++;
			outputfilename = argv[i];
		}
		else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
		{
			verbose = true;
		}
		else
			Error("Unknown option \"%s\"\n", argv[i]);
	}

	// fixme: this is a bit rubbish
	ReadMapFile(argv[i]);
}


// BuildTreeFromMapPolys
static void TreeTest()
{
	bsptree_t *tree;
	
	tree = BuildTree();
	
	Message("Building tree on map polys...\n");
	Message("%i nodes\n", tree->numnodes);
	Message("%i leaves\n", tree->numleafs);
	Message("%i tree depth\n", tree->depth);
	
	{
		WriteBinary(tree);
	}
}

int main(int argc, char *argv[])
{
	DebugInit();
	
	ProcessCommandLine(argc, argv);
	
	TreeTest();
	
	extern void TestPolygonForPlane();
	TestPolygonForPlane();

	
	DebugShutdown();
	
	return 0;
}
