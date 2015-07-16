#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#include "vec3.h"
#include "plane.h"
#include "polygon.h"

typedef struct mapface_s
{
	struct mapface_s	*next;
	polygon_t		*polygon;
	vec3			normal;
	
} mapface_t;

typedef struct mapdata_s
{
	struct mapface_s	*faces;
	int			numfaces;
	
} mapdata_t;

// nodes
typedef struct bspnode_s
{
	struct bspnode_s	*next;
	struct bspnode_s	*treenext;
	struct bspnode_s	*leafnext;
	struct bspnode_s	*parent;
	struct bspnode_s	*children[2];
	struct bsptree_s	*tree;
	
	// the node split plane
	plane_t			plane;
	
} bspnode_t;

// tree
typedef struct bsptree_s
{
	bspnode_t	*nodes;
	int		numnodes;
	int		numleafs;
	int		depth;
	
	bspnode_t	*root;
	bspnode_t	*leafs;
	
} bsptree_t;

extern const char	*outputfilename;
extern mapdata_t	*mapdata;

// toolib imports
void Error(const char *error, ...);
void Warning(const char *warning, ...);
void Message(const char *format, ...);

FILE *FileOpenBinaryRead(const char *filename);
FILE *FileOpenBinaryWrite(const char *filename);
FILE *FileOpenTextRead(const char *filename);
FILE *FileOpenTextWrite(const char *filename);
bool FileExists(const char *filename);
int FileSize(FILE *fp);
void FileClose(FILE *fp);
void WriteFile(const char *filename, void *data, int numbytes);
int ReadFile(const char* filename, void **data);

// debug.cpp
extern FILE *debugfp;
void DebugPrintfPolygon(FILE *fp, polygon_t *p);
void DebugWritePolygon(FILE *fp, polygon_t *p);
void DebugWriteWireFillPolygon(FILE *fp, polygon_t *p);
void DebugInit();
void DebugShutdown();

// main
void *Malloc(int numbytes);
void *MallocZeroed(int numbytes);

// token.cpp
char* ReadToken(FILE *fp);

// map file
void ReadMapFile(char *filename);

// bsp tree
bsptree_t *BuildTree();

// output functions
void WriteBinary(bsptree_t *tree);




