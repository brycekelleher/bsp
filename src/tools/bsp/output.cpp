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

static void EmitHeader(const char *symbol, FILE * fp)
{
	char header[8] = { 0 };
	strncpy(header, symbol, 8);
	WriteBytes(header, 8, fp);
}

#if 0
// emit nodes in pre-order
static int EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if (!n)
		return;

	children[0] = EmitNode(n->children[0]);
	children[1] = EmitNode(n->children[1]);

	EmitInt(children[0]);
	EmitInt(children[1]);

	// write the node data
	EmitNodeData();
}
#endif

#if 0
// emit nodes in post-order in place
static void EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if (!n)
		return;

	// write the node data
	EmitNodeData();

	// write a flag that indicates whether the node has children
	EmitInt((n->children[0] != NULL ? 1 : 0), fp);
	EmitInt((n->children[1] != NULL ? 1 : 0), fp);

	// emit the child nodes
	EmitNode(n->children[0], fp);
	EmitNode(n->children[1], fp);
}
#endif

#if 0
// emit nodes in post-order. This order must match how the nodes were numbered
// an alternative would be to write these out iteratively and fseek to the correct position
static void EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if (!n)
		return;
	
	// emit the child node numbers
	EmitInt((n->children[0] ? n->children[0]->nodenumber : -1), fp);
	EmitInt((n->children[1] ? n->children[1]->nodenumber : -1), fp);

#if 1
	// write the node data (disable this to check the node linkage)
	EmitPlane(n->plane, fp);
	EmitBox3(n->box, fp);
	EmitInt((n->area ? n->area->areanumber : -1), fp);
	EmitInt(n->empty ? 1 : 0, fp);
#endif
}

static void EmitNodeBlock(bsptree_t *tree, FILE *fp)
{
	NumberNodesRecursive(tree->root, 0);

	EmitInt(tree->numnodes, fp);
	EmitInt(tree->numleafs, fp);
	
	for (int i = 0; i < tree->numnodes; i++)
	{
		for (bspnode_t *n = tree->nodes; n; n = n->treenext)
			if (n->nodenumber == i)
				break;

		EmitNode(n, fp);
	}
}
#endif

static int NumberNodesRecursive(bspnode_t *n, int number)
{
	if (!n)
		return number;

	n->nodenumber = number++;
	number = NumberNodesRecursive(n->children[0], number);
	number = NumberNodesRecursive(n->children[1], number);

	return number;
}

// search for node with number 'num' and emit the node data into the file stream
// fixme: split this into a search and emit operation rather than combining both?
static void EmitNode(int num, bspnode_t *n, FILE *fp)
{
	// termination guard
	if (!n)
		return;

	// keep searching for the node in the tree
	if (n->nodenumber != num)
	{
		EmitNode(num, n->children[0], fp);
		EmitNode(num, n->children[1], fp);
		return;
	}

	// emit the child node numbers
	EmitInt((n->children[0] ? n->children[0]->nodenumber : -1), fp);
	EmitInt((n->children[1] ? n->children[1]->nodenumber : -1), fp);

#if 1
	// write the node data (disable this to check the node linkage)
	EmitPlane(n->plane, fp);
	EmitBox3(n->box, fp);
	EmitInt((n->area ? n->area->areanumber : -1), fp);
	EmitInt(n->empty ? 1 : 0, fp);
#endif
}

static void EmitNodeBlock(bsptree_t *tree, FILE *fp)
{
	NumberNodesRecursive(tree->root, 0);

	EmitHeader("nodes", fp);

	EmitInt(tree->numnodes, fp);
	EmitInt(tree->numleafs, fp);
	
	for (int i = 0; i < tree->numnodes; i++)
		EmitNode(i, tree->root, fp);
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
	NumberAreasRecursive(tree->areas, 0);

	EmitInt(tree->numareas, fp);
	for (area_t *a = tree->areas; a; a = a->next)
		EmitArea(a, fp);
}

void WriteBinary(bsptree_t *tree)
{
	Message("Writing binary \"%s\"...\n", outputfilename);
	
	FILE *fp = FileOpenBinaryWrite(outputfilename);
	
	EmitNodeBlock(tree, fp);

	//EmitAreaBlock(tree, fp);

	//EmitPortalBlock(tree, fp);

	//EmitAreaSurfaces(tree, fp);
}

