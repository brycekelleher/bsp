#include "bsp.h"

// at 128 = 1m, 0.02 = 0.15625mm
extern const float CLIP_EPSILON		= 0.2f;
extern const float AREA_EPSILON		= 0.1f;
extern const float PLANAR_EPSILON	= 0.2f;		// same as clip epsilon
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
// Debugging functions

void PrintPolygon(polygon_t *p)
{
	printf("polygon %p, numvertices=%i, maxvertices=%i\n", p, p->numvertices, p->maxvertices);
	for(int i = 0; i < p->numvertices; i++)
		printf("vertex %i: (%f %f %f)\n",
			i,
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
}

void PrintNode(bspnode_t *n)
{
	printf("plane=(%f, %f, %f, %f)", n->plane[0], n->plane[1], n->plane[2], n->plane[3]);
	printf(" ");
	printf("children=(%p, %p)", n->children[0], n->children[1]);
	printf("\n");
}

// ==============================================
// Main

static void PrintUsage()
{
	printf( "[-v] [-o outputfile] file ...\n");
}

static void ProcessEnvVars()
{}

// BuildTreeFromMapPolys
static void ProcessModel()
{
	bsptree_t *tree;
	
	Message("Processing model...\n");

	tree = BuildTree();
	
	MarkEmptyLeafs(tree);

	BuildPortals(tree);

	BuildAreas(tree);
#if 1
	extern void BuildAreaModels(bsptree_t *tree);
	BuildAreaModels(tree);
#endif

	WriteBinary(tree);
}

static void ProcessCommandLine(int argc, char *argv[])
{
	int i;
	
	if(argc == 1)
	{
		PrintUsage();
		exit(EXIT_SUCCESS);
	}

	for(i = 1; i < argc && argv[i][0] == '-'; i++)
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
