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

static void EmitString(FILE *fp, const char *format, ...)
{
	va_list valist;
	char buffer[2048];
	
	va_start(valist, format);
	vsprintf(buffer, format, valist);
	va_end(valist);

	EmitInt(strlen(buffer), fp);
	fprintf(fp, "%s", buffer);
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

	// write the node data
	EmitPlane(n->plane, fp);
	EmitBox3(n->box, fp);
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

static void EmitPortalBlock(bsptree_t *tree, FILE *fp)
{
	EmitHeader("portals", fp);

	EmitInt(tree->numportals, fp);
	for (portal_t *p = tree->portals; p; p = p->treenext)
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
}

static void EmitAreaLeaves(area_t *a, FILE *fp)
{
	// emit the leaf numbers
	EmitInt(a->numleafs, fp);
	for (bspnode_t *n = a->leafs; n; n = n->areanext)
		EmitInt(n->nodenumber, fp);
}

static void EmitArea(area_t *a, FILE *fp)
{
	EmitAreaLeaves(a, fp);
}

static void EmitAreaBlock(bsptree_t *tree, FILE *fp)
{
	NumberAreasRecursive(tree->areas, 0);

	EmitHeader("areas", fp);

	EmitInt(tree->numareas, fp);

	for (area_t *a = tree->areas; a; a = a->next)
		EmitArea(a, fp);
}

static void EmitAreaRenderModel(area_t *a, FILE *fp)
{
	EmitHeader("rmodel", fp);
	EmitString(fp, "area%04i", a->areanumber);

	BeginMesh();
	for (areatri_t *t = a->trilist->head; t; t = t->next)
	{
		// fixme: function to create these
		meshvertex_t v0, v1, v2;
		v0.xyz = t->vertices[0];
		v0.normal = t->normals[0];
		v1.xyz = t->vertices[1];
		v1.normal = t->normals[1];
		v2.xyz = t->vertices[2];
		v2.normal = t->normals[2];

		InsertTri(v0, v1, v2);
	}
	EndMesh();

	// emit the vertex block
	int numvertices = NumVertices();
	EmitInt(numvertices, fp);
	for (int i = 0; i < numvertices; i++)
	{
		meshvertex_t v = GetVertex(i);
		EmitFloat(v.xyz[0], fp);
		EmitFloat(v.xyz[1], fp);
		EmitFloat(v.xyz[2], fp);
		EmitFloat(v.normal[0], fp);
		EmitFloat(v.normal[1], fp);
		EmitFloat(v.normal[2], fp);
	}

	// emit the index block
	int numindicies = NumIndicies();
	EmitInt(numindicies, fp);
	for (int i = 0; i < numindicies; i++)
		EmitInt(GetIndex(i), fp);
}

static void EmitAreaRenderModels(bsptree_t *tree, FILE *fp)
{
	for (area_t *a = tree->areas; a; a = a->next)
		EmitAreaRenderModel(a, fp);
}

static void EmitStaticRenderModel(smodel_t *m, FILE *fp)
{
	static int count = 0;

	EmitHeader("rmodel", fp);
	EmitString(fp, "staticmodel%04i", count);
	
	// emit the vertex block
	EmitInt(m->numvertices, fp);
	for (int i = 0; i < m->numvertices; i++)
	{
		EmitFloat(m->vertices[i][0], fp);
		EmitFloat(m->vertices[i][1], fp);
		EmitFloat(m->vertices[i][2], fp);
		EmitFloat(0.0f, fp);
		EmitFloat(0.0f, fp);
		EmitFloat(1.0f, fp);
	}

	// emit the index block
	EmitInt(m->numindicies, fp);
	for (int i = 0; i < m->numindicies; i++)
		EmitInt(m->indicies[i], fp);
}

static void EmitStaticRenderModels(FILE *fp)
{
	for(smodel_t *m = smodels; m; m = m->next)
		EmitStaticRenderModel(m, fp);
}

void WriteBinary(bsptree_t *tree)
{
	Message("Writing binary \"%s\"...\n", outputfilename);
	
	FILE *fp = FileOpenBinaryWrite(outputfilename);
	
	EmitNodeBlock(tree, fp);

	EmitAreaBlock(tree, fp);

	EmitPortalBlock(tree, fp);

	EmitAreaRenderModels(tree, fp);

	EmitStaticRenderModels(fp);
}

