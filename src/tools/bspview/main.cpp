#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include "vec3.h"
#include "box3.h"
#include "plane.h"


// fixme....
//void Error(const char *error, ...);
//void Warning(const char *warning, ...);
//void Message(const char *format, ...);

FILE *FileOpenBinaryRead(const char *filename);
FILE *FileOpenBinaryWrite(const char *filename);
FILE *FileOpenTextRead(const char *filename);
FILE *FileOpenTextWrite(const char *filename);
bool FileExists(const char *filename);
int FileSize(FILE *fp);
void FileClose(FILE *fp);
void WriteFile(const char *filename, void *data, int numbytes);
int ReadFile(const char* filename, void **data);
void WriteBytes(void *data, int numbytes, FILE *fp);
int ReadBytes(void *buf, int numbytes, FILE *fp);

// ==============================================
// memory allocation

#define MEM_ALLOC_SIZE	32 * 1024 * 1024

typedef struct memstack_s
{
	unsigned char mem[MEM_ALLOC_SIZE];
	int allocated;

} memstack_t;

static memstack_t memstack;

void *Mem_Alloc(int numbytes)
{
	unsigned char *mem;
	
	if(memstack.allocated + numbytes > MEM_ALLOC_SIZE)
	{
		printf("Error: Mem: no free space available\n");
		abort();
	}

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	memset(mem, 0, numbytes);

	return mem;
}

void Mem_FreeStack()
{
	memstack.allocated = 0;
}

// ==============================================
// model loading

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

typedef struct surfvertex_s
{
	vec3		xyz;

} surfvertex_t;

// a triangle surface
typedef struct surf_s
{
	struct surf_s*	next;

	int		numvertices;
	surfvertex_t	*vertices;

	//int		numindicies;
	//surfvertex_t	*indicies;

} surf_t;

typedef struct bspnode_s
{
	plane_t		plane;
	box3		box;
	int		areanum;
	struct bspnode_s*	children[2];
	struct bspnode_s*	areanext;
	struct area_s*		area;

} bspnode_t;

typedef struct portal_s
{
	struct portal_s		*next;
	struct portal_s		*areanext;
	bspnode_t		*srcleaf;
	bspnode_t		*dstleaf;

	int 			numvertices;
	vec3			*vertices;

} portal_t;

typedef struct area_s
{
	int		numportals;
	portal_t	*portals;

	int		numleafs;
	bspnode_t	*leafs;

	int		numsurfaces;
	surf_t		*surfaces;

} area_t;

// model data
static area_t		*areas;
static bspnode_t	*nodes;
static portal_t		*portals;

static void LoadNodes(FILE *fp)
{
	int numnodes = ReadInt(fp);
	int numleafs = ReadInt(fp);
	nodes = (bspnode_t*)Mem_Alloc(numnodes * sizeof(bspnode_t));

	for (int i = 0; i < numnodes; i++)
	{
		bspnode_t *n = nodes + i;

		// fixme: move into another function
		n->plane[0] = ReadFloat(fp);
		n->plane[1] = ReadFloat(fp);
		n->plane[2] = ReadFloat(fp);
		n->plane[3] = ReadFloat(fp);

		// fixme: move into another function
		n->box.min[0] = ReadFloat(fp);
		n->box.min[1] = ReadFloat(fp);
		n->box.min[2] = ReadFloat(fp);
		n->box.max[0] = ReadFloat(fp);
		n->box.max[1] = ReadFloat(fp);
		n->box.max[2] = ReadFloat(fp);

		// fixme: a -1 areanum gives a bad pointer
		int areanum = ReadInt(fp);
		n->area = areas + areanum;

		int childnum[2];
		childnum[0] = ReadInt(fp);
		childnum[1] = ReadInt(fp);
		n->children[0] = nodes + childnum[0];
		n->children[1] = nodes + childnum[1];
	}
}

static portal_t *LoadPortal(FILE *fp)
{
	portal_t *p = (portal_t*)Mem_Alloc(sizeof(portal_t));
	p->next = portals;
	portals = p;

	int srcleaf = ReadInt(fp);
	p->srcleaf = nodes + srcleaf;

	int dstleaf = ReadInt(fp);
	p->dstleaf = nodes + dstleaf;

	p->numvertices = ReadInt(fp);
	p->vertices = (vec3*)Mem_Alloc(p->numvertices * sizeof(vec3));
	for (int i = 0; i < p->numvertices; i++)
	{
		p->vertices[i][0] = ReadFloat(fp);
		p->vertices[i][1] = ReadFloat(fp);
		p->vertices[i][2] = ReadFloat(fp);
	}

	return p;
}

static surf_t *LoadSurface(FILE *fp)
{
	surf_t *s = (surf_t*)Mem_Alloc(sizeof(surf_t));

	s->numvertices = ReadInt(fp);
	s->vertices = (surfvertex_t*)Mem_Alloc(s->numvertices * sizeof(surfvertex_t));

	for (int i = 0; i < s->numvertices; i++)
	{
		s->vertices[i].xyz[0] = ReadFloat(fp);
		s->vertices[i].xyz[1] = ReadFloat(fp);
		s->vertices[i].xyz[2] = ReadFloat(fp);
	}

	return s;
}

static void LoadAreas(FILE *fp)
{
	int numareas = ReadInt(fp);
	areas = (area_t*)Mem_Alloc(numareas * sizeof(area_t));

	for (int i = 0; i < numareas; i++)
	{
		area_t *a = areas + i;

		// read the leaf nodes
		a->numleafs = ReadInt(fp);
		for (int j = 0; j < a->numleafs; j++)
		{
			int leafnum = ReadInt(fp);
			bspnode_t *l = nodes + leafnum;
			l->area = a;
			
			// link into the area list
			l->areanext = a->leafs;
			a->leafs = l;
		}

		// read the portals
		a->numportals = ReadInt(fp);
		for (int j = 0; j < a->numportals; j++)
		{
			portal_t *p = LoadPortal(fp);

			// linkt the portal into the area list
			p->areanext = a->portals;
			a->portals = p;

			// fixme: link into the tree list?
		}

		// read the area surfaces
		a->numsurfaces = ReadInt(fp);
		for (int j = 0; j < a->numsurfaces; j++)
		{
			surf_t *s = LoadSurface(fp);

			// link the surface into the area list
			s->next = a->surfaces;
			a->surfaces = a->surfaces;
		}
	}
}

static void LoadModel(FILE *fp)
{
	LoadNodes(fp);
	LoadAreas(fp);
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("bspview bspfile\n");
		exit(EXIT_SUCCESS);
	}

	FILE *fp = FileOpenBinaryRead(argv[1]);
	LoadModel(fp);
	FileClose(fp);

	return 0;
}

