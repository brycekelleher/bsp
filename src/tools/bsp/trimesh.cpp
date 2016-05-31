#include "bsp.h"

// ________________________________________________________________________________ 
// Mesh builder

static int numvertices;
static int maxvertices;
static meshvertex_t *vertices;

static int numindicies;
static int maxindicies;
static int *indicies;

static void ExpandVertexList(int count)
{
	maxvertices = count;
	vertices = (meshvertex_t*)realloc(vertices, maxvertices * sizeof(meshvertex_t));
}

static void ExpandIndexList(int count)
{
	maxindicies = count;
	indicies = (int*)realloc(indicies, maxindicies * sizeof(int));
}

static int InsertVertex(meshvertex_t v)
{
	if (numvertices == maxvertices)
		ExpandVertexList(maxvertices * 2);

	vertices[numvertices++] = v;
	return numvertices - 1;
}

static void InsertIndex(int index)
{
	if (numindicies == maxindicies)
		ExpandIndexList(maxindicies * 2);
	
	indicies[numindicies++] = index;
}

#if 0
static int LookupVertex(meshvertex_t v)
{
	for (int i = 0; i < numvertices; i++)
		if (vertices[i].xyz == v.xyz && 
			vertices[i].normal == v.normal)
			return i;

	return InsertVertex(v);
}
#endif
#if 1
static int LookupVertex(meshvertex_t v)
{
	for (int i = 0; i < numvertices; i++)
		if ((Length(vertices[i].xyz - v.xyz) < 0.01f) &&
			vertices[i].normal == v.normal)
			return i;

	return InsertVertex(v);
}
#endif

void BeginMesh()
{
	if (vertices)
	{
		free(vertices);
		vertices = NULL;
	}

	numvertices = 0;
	maxvertices = 1;
	ExpandVertexList(maxvertices);

	if (indicies)
	{
		free(indicies);
		indicies = NULL;
	}

	numindicies = 0;
	maxindicies = 1;
	ExpandIndexList(maxindicies);
}

void EndMesh()
{
}

void InsertTri(meshvertex_t v0, meshvertex_t v1, meshvertex_t v2)
{
	InsertIndex(LookupVertex(v0));
	InsertIndex(LookupVertex(v1));
	InsertIndex(LookupVertex(v2));
}

int NumVertices()
{
	return numvertices;
}

int NumIndicies()
{
	return numindicies;
}

meshvertex_t GetVertex(int i)
{
	return vertices[i];
}

int GetIndex(int i)
{
	return indicies[i];
}
