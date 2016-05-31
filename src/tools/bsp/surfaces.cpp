#include "bsp.h"

static void PrintLeafFaces(bsptree_t *tree)
{
	for (area_t *a = tree->areas; a; a = a->next)
		for (leafface_t *lf = a->leaffaces; lf; lf = lf->areanext)
			PrintPolygon(lf->polygon);
}

static void WriteGLViewFaces(bsptree_t *tree)
{
	FILE *fp = fopen("/tmp/f", "wb");
	if (!fp)
		return;

	fprintf(fp, "flush\n");
	for (area_t *a = tree->areas; a; a = a->next)
		for (leafface_t *lf = a->leaffaces; lf; lf = lf->areanext)
			DebugWriteWireFillPolygon(fp, lf->polygon);
	
	fclose(fp);
}

//________________________________________________________________________________
// Debug code for generating faces

static FILE *fifofp;

// fixme: should include map faces?
static void EmitAreaVertex(vec3 v)
{
	fprintf(fifofp, "%f %f %f\n", v[0], v[1], v[2]);
}

static void EmitAreaSurface(area_t *a)
{
	int numtris = 0;
	for (leafface_t *f = a->leaffaces; f; f = f->areanext)
		numtris += f->polygon->numvertices - 2;

	//for (int i = 0; i < count; i++)
	//	EmitIndex(i);

	for (leafface_t *f = a->leaffaces; f; f = f->areanext)
		for (int i = 0; i < f->polygon->numvertices - 2; i++)
		{
			//EmitAreaVertex(f->polygon->vertices[0]);
			//EmitAreaVertex(f->polygon->vertices[i + 1]);
			//EmitAreaVertex(f->polygon->vertices[i + 2]);

			fifofp = fopen("/tmp/f", "wb");
			if (fifofp)
			{
				fprintf(fifofp, "polyline 4\n");
				EmitAreaVertex(f->polygon->vertices[0]);
				EmitAreaVertex(f->polygon->vertices[i + 1]);
				EmitAreaVertex(f->polygon->vertices[i + 2]);
				EmitAreaVertex(f->polygon->vertices[0]);
				fclose(fifofp);
			}
		}
}

static void EmitAreaGeometry(bsptree_t *tree)
{
	fprintf(fifofp, "flush\n");

	for (area_t *a = tree->areas; a; a = a->next)
	{
		if (!a->leaffaces)
			continue;

		EmitAreaSurface(a);
	}
}

//________________________________________________________________________________
// Leaf faces
// push every map face into the tree and store in the areas

static leafface_t *AllocLeafFace(area_t *area, bspnode_t *node, polygon_t *p)
{
	leafface_t *lf = (leafface_t*)MallocZeroed(sizeof(leafface_t));

	lf->leaf = node;
	lf->area = area;
	lf->polygon = p;

	// fixme: global leafface list for solving tjunc's?

	lf->areanext = area->leaffaces;
	area->leaffaces = lf;

	lf->nodenext = node->leaffaces;
	node->leaffaces = lf;

	return lf;
}

static void PushFaceIntoTree(bspnode_t *n, polygon_t *p)
{
	if (!n->children[0] && !n->children[1])
	{
		// only emit polygons into empty leafs
		if (!n->empty)
			return;

		AllocLeafFace(n->area, n, p);
		return;
	}

	int side = Polygon_OnPlaneSide(p, n->plane, CLIP_EPSILON);

	if (side == PLANE_SIDE_FRONT)
		PushFaceIntoTree(n->children[0], p);
	else if (side == PLANE_SIDE_BACK)
		PushFaceIntoTree(n->children[1], p);
	else if (side == PLANE_SIDE_ON)
	{
		float dot = Dot(n->plane.GetNormal(), Polygon_Normal(p));

		// map 0 to the front child and 1 to the back child
		int facing = (dot > 0.0f ? 0 : 1);

		PushFaceIntoTree(n->children[facing], p);
	}
	else if (side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(p, n->plane, CLIP_EPSILON, &f, &b);

		PushFaceIntoTree(n->children[0], f);
		PushFaceIntoTree(n->children[1], b);
	}
}

void BuildFaceFragments(bsptree_t *tree)
{
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// fixme: need a method to filter only structural polygons
		if (f->areahint)
			continue;

		polygon_t *polygon = Polygon_Copy(f->polygon);

		PushFaceIntoTree(tree->root, polygon);
	}
}


//________________________________________________________________________________
// Area models
// consumes a list of leaffaces and produces a list of areatris

static trilist_t *BuildAreaTriList(area_t *a)
{
	trilist_t *trilist = CreateTriList();

	for (leafface_t *lf = a->leaffaces; lf; lf = lf->areanext)
	{
		polygon_t *p = lf->polygon;
		for (int i = 0; i < p->numvertices - 2; i++)
		{
			areatri_t *t = AllocAreaTri();

			t->vertices[0] = p->vertices[0];
			t->vertices[1] = p->vertices[(i + 1) % p->numvertices];
			t->vertices[2] = p->vertices[(i + 2) % p->numvertices];

			Append(trilist, t);
		}
	}

	return trilist;
}

void BuildAreaModels(bsptree_t *tree)
{
	BuildFaceFragments(tree);

	// build the area trilists
	for (area_t *a = tree->areas; a; a = a->next)
	{
		trilist_t *trilist = BuildAreaTriList(a);

#if 1		
		// fix the t-junctions
		trilist_t *fixedlist = FixTJunctions(trilist);
		FreeTriList(trilist);
		trilist = fixedlist;
#endif

		// calculate per vertex normals
		trilist = CalculateNormals(trilist);

		a->trilist = trilist;
	}
}

