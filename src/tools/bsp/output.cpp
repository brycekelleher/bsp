#include "bsp.h"

static int NumberAreasRecursive(area_t *a, int number)
{
	if (!a)
		return number;

	a->areanumber = number++;
	return NumberAreasRecursive(a->next, number);
}

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

static int NumberNodesRecursive(bspnode_t *n, int number)
{
	if (!n)
		return number;

	n->nodenumber = number++;
	number = NumberNodesRecursive(n->children[0], number);
	number = NumberNodesRecursive(n->children[1], number);

	return number;
}

// emit nodes in pre-order. This order must match how the nodes were numbered
// an alternative would be to write these out iteratively and fseek to the correct position
static void EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if (!n)
		return;
	
	// write the node data
	{
// disable this to check the node linkage
#if 1
		EmitPlane(n->plane, fp);
		EmitBox3(n->box, fp);
		EmitInt((n->area ? n->area->areanumber : -1), fp);
#endif
	}

	// emit the child node numbers
	EmitInt((n->children[0] ? n->children[0]->nodenumber : -1), fp);
	EmitInt((n->children[1] ? n->children[1]->nodenumber : -1), fp);

	// emit the child nodes
	EmitNode(n->children[0], fp);
	EmitNode(n->children[1], fp);
}

static void EmitNodeBlock(bsptree_t *tree, FILE *fp)
{
	EmitInt(tree->numnodes, fp);
	EmitInt(tree->numleafs, fp);
	
	EmitNode(tree->root, fp);
}

static void EmitPortal(portal_t *p, FILE *fp)
{
	EmitInt(p->srcleaf->nodenumber, fp);
	EmitInt(p->dstleaf->nodenumber, fp);

	polygon_t *polygon = p->polygon;
	EmitInt(polygon->numvertices, fp);
	for (int i = 0; i < polygon->numvertices; i++)
	{
		EmitFloat(polygon->vertices[i][0], fp);
		EmitFloat(polygon->vertices[i][1], fp);
		EmitFloat(polygon->vertices[i][2], fp);
	}
}

static void EmitAreaPortals(area_t *a, FILE *fp)
{
	portal_t *p;

	EmitInt(a->numportals, fp);
	for (p = a->portals; p; p = p->areanext)
		EmitPortal(p, fp);
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

static void EmitAreaSurfaces(area_t *a, FILE *fp)
{
	trisurf_t *s = a->trisurf;
	if (!s)
	{
		EmitInt(0, fp);
		return;
	}

	EmitInt(1, fp);
	EmitTriSurf(s, fp);
}


static void EmitArea(area_t *a, FILE *fp)
{
	// emit the leaf numbers
	EmitInt(a->numleafs, fp);
	for (bspnode_t *n = a->leafs; n; n = n->areanext)
		EmitInt(n->nodenumber, fp);

	// emit the area portal
	EmitAreaPortals(a, fp);

	// emit the render surfaces
	EmitAreaSurfaces(a, fp);
}

static void EmitAreaBlock(bsptree_t *tree, FILE *fp)
{
	EmitInt(tree->numareas, fp);
	for (area_t *a = tree->areas; a; a = a->next)
		EmitArea(a, fp);
}

static void EmitTree(bsptree_t *tree, FILE *fp)
{
	NumberNodesRecursive(tree->root, 0);
	NumberAreasRecursive(tree->areas, 0);

	EmitNodeBlock(tree, fp);
	EmitAreaBlock(tree, fp);
	//EmitPortalBlock(tree, fp);
	//EmitAreaSurfaces(tree, fp);
}

void WriteBinary(bsptree_t *tree)
{
	Message("Writing binary \"%s\"...\n", outputfilename);
	
	FILE *fp = FileOpenBinaryWrite(outputfilename);
	
	EmitTree(tree, fp);
}

