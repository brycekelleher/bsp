#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "vec3.h"
#include "box3.h"
#include "plane.h"
#include "polygon.h"

extern const float CLIP_EPSILON;
extern const float AREA_EPSILON;
extern const float PLANAR_EPSILON;
extern const float MAX_VERTEX_SIZE;

// ________________________________________________________________________________ 
// trilist
// this is the main sequences-as-standard-interfaces element

typedef struct areatri_s
{
	struct areatri_s	*next;
	vec3			vertices[3];
	vec3			normals[3];
} areatri_t;

typedef struct trilist_s
{
	areatri_t	*head;
	areatri_t	*tail;
} trilist_t;

areatri_t *AllocAreaTri();
areatri_t *Copy(areatri_t *t);
trilist_t *CreateTriList();
void FreeTriList(trilist_t *l);
void Append(trilist_t *l, areatri_t *t);
areatri_t *Head(trilist_t *l);
int Length(trilist_t *l);

trilist_t *FixTJunctions(trilist_t *trilist);
trilist_t *CalculateNormals(trilist_t *trilist);

// ________________________________________________________________________________ 
// trimesh

// mesh
typedef struct meshvertex_s
{
	vec3	xyz;
	vec3	normal;

} meshvertex_t;

void BeginMesh();
void EndMesh();
void InsertTri(meshvertex_t v0, meshvertex_t v1, meshvertex_t v2);
int NumVertices();
int NumIndicies();
meshvertex_t GetVertex(int i);
int GetIndex(int i);

// ________________________________________________________________________________ 

typedef struct mapface_s
{
	struct mapface_s	*next;
	polygon_t		*polygon;
	plane_t			plane;
	box3			box;
	bool			areahint;
	
} mapface_t;

typedef struct mapdata_s
{
	struct mapface_s	*faces;
	int			numfaces;
	int			numareahints;
	
} mapdata_t;

typedef struct leafface_s
{
	struct leafface_s	*areanext;
	struct leafface_s	*nodenext;
			
	struct bspnode_s	*leaf;
	struct area_s		*area;
	polygon_t	*polygon;

} leafface_t;

typedef struct portal_s
{
	struct portal_s		*treenext;
	struct portal_s		*leafnext;
	struct portal_s		*areanext;

	struct bspnode_s	*srcleaf;
	struct bspnode_s	*dstleaf;
	
	polygon_t		*polygon;
	bool			areahint;

} portal_t;

typedef struct area_s
{
	struct area_s		*next;

	struct bspnode_s	*leafs;
	int			numleafs;

	// area portals
	struct portal_s		*portals;
	int			numportals;

	// area render surfaces
	struct leafface_s	*leaffaces;
	struct trilist_s	*trilist;

	// output number
	int			areanumber;

} area_t;

// nodes
typedef struct bspnode_s
{
	struct bspnode_s	*children[2];
	struct bspnode_s	*parent;
	struct bspnode_s	*leafnext;
	struct bspnode_s	*globalnext;
	struct bspnode_s	*treenext;

	struct bsptree_s	*tree;
	
	// the node split plane
	plane_t			plane;
	bool			areahint;
	
	// the node bounding box
	box3			box;

	// portals if this node is a leaf
	struct portal_s		*portals;
	int			numportals;

	// areas
	struct area_s		*area;
	struct bspnode_s	*areanext;

	// leaf faces
	struct leafface_s	*leaffaces;

	// flags
	bool			empty;

	// output number
	int			nodenumber;
	
} bspnode_t;

// tree
typedef struct bsptree_s
{
	bspnode_t	*nodes;
	int		numnodes;
	int		numleafs;
	int		mindepth;
	int		maxdepth;
	
	bspnode_t	*root;
	bspnode_t	*leafs;
	
	portal_t	*portals;
	int		numportals;

	area_t		*areas;
	int		numemptyareas;
	int		numareas;

} bsptree_t;

extern const char	*outputfilename;
extern mapdata_t	*mapdata;

// ________________________________________________________________________________ 
// new static model stuff

typedef struct smodel_s
{
	struct smodel_s *next;
	int numvertices;
	vec3 *vertices;

	int numindicies;
	int *indicies;

} smodel_t;


extern smodel_t		*smodels;

// ________________________________________________________________________________ 


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
void WriteBytes(void *data, int numbytes, FILE *fp);
int ReadBytes(void *buf, int numbytes, FILE *fp);

// debug.cpp
extern FILE *debugfp;
void DebugPrintfPolygon(FILE *fp, polygon_t *p);
void DebugWriteColor(FILE *fp, float *rgb);
void DebugWritePolygon(FILE *fp, polygon_t *p);
void DebugWriteWireFillPolygon(FILE *fp, polygon_t *p);
void DebugInit();
void DebugShutdown();
void DebugWritePortalFile(bsptree_t *tree);
void DebugWritePortalPolygon(bsptree_t *tree, polygon_t *p);
void DebugEndLeafPolygons();
void DebugDumpAreaSurfaces(bsptree_t *tree);

// main
void *Malloc(int numbytes);
void *MallocZeroed(int numbytes);
void PrintPolygon(polygon_t *p);
void PrintNode(bspnode_t *n);

// map file
void ReadMap(char *filename);

// bsp tree
bsptree_t *BuildTree();

// portals
void BuildPortals(bsptree_t* tree);

// areas
void MarkEmptyLeafs(bsptree_t *tree);
void BuildAreas(bsptree_t *tree);

// output functions
void WriteBinary(bsptree_t *tree);


