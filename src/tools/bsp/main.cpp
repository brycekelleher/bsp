#include "bsp.h"

// at 128 = 1m, 0.02 = 0.15625mm
extern const float CLIP_EPSILON		= 0.2f;
extern const float AREA_EPSILON		= 0.1f;
extern const float PLANAR_EPSILON	= 0.01f;
extern const float MAX_VERTEX_SIZE	= 4096.0f;

const char	*outputfilename = "out.bsp";
static bool	verbose = false;

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

	exit(EXIT_SUCCESS);
}

static void ProcessEnvVars()
{}

// BuildTreeFromMapPolys
static void ProcessModel()
{
	bsptree_t *tree;
	
	Message("Processing model...\n");

	tree = BuildTree();
	
	Message("%i nodes\n", tree->numnodes);
	Message("%i leaves\n", tree->numleafs);
	Message("%i tree depth\n", tree->depth);
	
	MarkEmptyLeafs(tree);

	BuildPortals(tree);

	BuildAreas(tree);

	WriteBinary(tree);
}

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

	// eventually this could loop through all source files
	// the higher level construct would be of a "bsp model"
	// allowing multiple bsp models to be packed into a single file?
	ReadMap(argv[i]);

	ProcessModel();
}

int main(int argc, char *argv[])
{
	DebugInit();
	
	ProcessEnvVars();

	ProcessCommandLine(argc, argv);
	
	DebugShutdown();
	
	return 0;
}
