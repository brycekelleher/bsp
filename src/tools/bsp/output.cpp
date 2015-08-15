#include "bsp.h"

static void EmitInt(int i, FILE* fp)
{
	WriteBytes(&i, sizeof(int), fp);
}

static void EmitFloat(float f, FILE *fp)
{
	WriteBytes(&f, sizeof(float), fp);
}

static void EmitPlane(plane_t plane, FILE *fp)
{
	EmitFloat(plane.a, fp);
	EmitFloat(plane.b, fp);
	EmitFloat(plane.c, fp);
	EmitFloat(plane.d, fp);
}

static void EmitBox3(box3 box, FILE *fp)
{
	EmitFloat(box.min[0], fp);
	EmitFloat(box.min[1], fp);
	EmitFloat(box.min[2], fp);
	EmitFloat(box.max[0], fp);
	EmitFloat(box.max[1], fp);
	EmitFloat(box.max[2], fp);
}

static void EmitPolygon(polygon_t *p, FILE *fp)
{
	EmitInt(p->numvertices, fp);

	for(int i = 0; i < p->numvertices; i++)
	{
		EmitFloat(p->vertices[i][0], fp);
		EmitFloat(p->vertices[i][1], fp);
		EmitFloat(p->vertices[i][2], fp);
	}
}

static void EmitPortal(portal_t *p, FILE *fp)
{
	EmitPolygon(p->polygon, fp);
}

static void EmitTriSurf(trisurf_t *s, FILE *fp)
{
	EmitInt(s->numvertices, fp);

	for (int i = 0; i < s->numvertices; i++)
	{
		EmitFloat(s->vertices[i][0], fp);
		EmitFloat(s->vertices[i][1], fp);
		EmitFloat(s->vertices[i][2], fp);
	}
}

static void EmitAreaSurfaces(bsptree_t *tree, FILE *fp)
{
	for (area_t *a = tree->areas; a; a = a->next)
	{
		trisurf_t *s = a->trisurf;
		if (!s)
			continue;

		EmitTriSurf(s, fp);
	}
}

static void EmitNodePortals(bspnode_t* n, FILE *fp)
{
	for (portal_t *p = n->portals; p; p = p->leafnext)
		EmitPortal(p, fp);
}

// emit nodes in postorder (leaves at start of file)
static int numnodes = 0;
static int EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if(!n)
	{
		return -1;
	}
	
	// emit the children
	int childnum[2];
	childnum[0] = EmitNode(n->children[0], fp);
	childnum[1] = EmitNode(n->children[1], fp);
	
	// write the node data
	{
		EmitInt(childnum[0], fp);
		EmitInt(childnum[1], fp);

// disable this to check the node linkage
#if 1
		EmitPlane(n->plane, fp);
		EmitBox3(n->box, fp);
		EmitNodePortals(n, fp);
#endif
	}
	
	int thisnode = numnodes++;
	return thisnode;
}

static void EmitTree(bsptree_t *tree, FILE *fp)
{
	EmitInt(tree->numnodes, fp);
	EmitInt(tree->numleafs, fp);
	
	int rootnode = EmitNode(tree->root, fp);

	EmitAreaSurfaces(tree, fp);
}

void WriteBinary(bsptree_t *tree)
{
	Message("Writing binary \"%s\"...\n", outputfilename);
	
	FILE *fp = FileOpenBinaryWrite(outputfilename);
	
	EmitTree(tree, fp);
}
