#include "bsp.h"

// ==============================================
// Surface code

typedef struct trisurf_t
{
	int	numtriangles;
	int	maxtriangles;
	vec3	*vertices;

} trisurf_t;

static trisurf_t *TS_New()
{
	return (trisurf_t*)MallocZeroed(sizeof(trisurf_t));
}

// fixme: reimplement malloc?
static void TS_AddTriangle(trisurf_t *s, vec3 v0, vec3 v1, vec3 v2)
{
	if (s->numtriangles + 1 >= s->maxtriangles)
	{
		s->maxtriangles *= 2;
		if(!s->maxtriangles)
			s->maxtriangles = 16;

		s->vertices = (vec3*)realloc(s->vertices, s->maxtriangles * 3 * sizeof(float));
	}

	vec3 *v = s->vertices + s->numtriangles++;
	*v++ = v0;
	*v++ = v1;
	*v++ = v2;
}

// ==============================================
// BSP surface code

// fan the polygon out into the trianglelist
static void AddPolygonToArea(bspnode_t *n, polygon_t* p)
{
	area_t *a =  (area_t*)n->area;

	// get the surface
	if (!a->trisurf)
		a->trisurf = (area_t*)TS_New();
	trisurf_t *s = (trisurf_t*)a->trisurf;

	for (int i = 0; i < p->numvertices - 2; i++)
		TS_AddTriangle(s, p->vertices[0], p->vertices[i + 1], p->vertices[i + 2]);
}

static void FilterPolygonIntoLeaf(bspnode_t *n, polygon_t *p)
{
	if (!n->children[0] && !n->children[1])
	{
		// only emit polygons into empty leafs
		if (!n->empty)
			return;

		AddPolygonToArea(n, p);
		return;
	}

	int side = Polygon_OnPlaneSide(p, n->plane, CLIP_EPSILON);

	if (side == PLANE_SIDE_FRONT)
		FilterPolygonIntoLeaf(n->children[0], p);
	else if (side == PLANE_SIDE_BACK)
		FilterPolygonIntoLeaf(n->children[1], p);
	else if (side == PLANE_SIDE_ON)
	{
		float dot = Dot(n->plane.GetNormal(), Polygon_Normal(p));

		// map 0 to the front child and 1 to the back child
		int facing = (dot > 0.0f ? 0 : 1);

		FilterPolygonIntoLeaf(n->children[facing], p);
	}
	else if (side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(p, n->plane, CLIP_EPSILON, &f, &b);

		FilterPolygonIntoLeaf(n, f);
		FilterPolygonIntoLeaf(n, b);
	}
}

static void PrintSurfaces(bsptree_t *tree)
{
	for (area_t *a = tree->areas; a; a = a->next)
	{
		trisurf_t *s = (trisurf_t*)a->trisurf;
		printf("area %p surface\n", a);

		int i;
		vec3 *v;
		for (i = 0, v = s->vertices; i < s->numtriangles * 3; i++, v++)
			printf("vertex %i: (%f %f %f)\n", i, (*v)[0], (*v)[1], (*v)[2]);
	}
}

// push every map face into the tree and store in the areas
void BuildSurfaces(bsptree_t *tree)
{
	// Iterate all map faces, filter them into the tree
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// Fixme: need a method to filter only structural polygons
		if (f->areahint)
			continue;

		polygon_t *polygon = Polygon_Copy(f->polygon);

		FilterPolygonIntoLeaf(tree->root, polygon);
	}

	PrintSurfaces(tree);
}

