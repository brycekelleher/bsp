#include "bsp.h"

// ==============================================
// Surface code

typedef struct trisurf_t
{
	int	numvertices;
	int	maxvertices;
	vec3	*vertices;

} trisurf_t;

static trisurf_t *TS_New()
{
	return (trisurf_t*)MallocZeroed(sizeof(trisurf_t));
}

// fixme: reimplement malloc?
static void TS_AddTriangle(trisurf_t *s, vec3 v0, vec3 v1, vec3 v2)
{
	if (s->numvertices + 3 >= s->maxvertices)
	{
		s->maxvertices *= 2;
		if(!s->maxvertices)
			s->maxvertices = 16;

		s->vertices = (vec3*)realloc(s->vertices, s->maxvertices * sizeof(s->vertices[0]));
	}

	vec3 *v = s->vertices + s->numvertices;
	s->numvertices += 3;

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

		if (!s)
			continue;

		printf("area %p surface\n", a);
		for (int i = 0; i < s->numvertices; i++)
			printf("vertex %i: (%f %f %f)\n", i, s->vertices[i][0], s->vertices[i][1], s->vertices[i][2]);
	}
}

// fixme: all this needs to be moved
// maps f in the range 0..1 to a HSV color
static void HSVToRGB(float rgb[3], float h, float s, float v)
{
	float r, g, b;
	
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
	switch(i % 6)
	{
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}
	
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

static void ColorMap(float rgb[3], float f)
{
	return HSVToRGB(rgb, f, 1.0f, 1.0f);
}

static void ColorMap(float rgb[3], int i, int n)
{
	return ColorMap(rgb, (float)i / n);
}

void DebugDumpAreaSurfaces(bsptree_t *tree)
{
	FILE *fp = FileOpenTextWrite("debug_surfaces.gld");

	int i = 0;
	for (area_t *a = tree->areas; a; a = a->next, i++)
	{
		trisurf_t *s = (trisurf_t*)a->trisurf;

		if (!s)
			continue;

		//fprintf(fp, "//area %p surface\n", a);
		{
			float rgb[3];
			ColorMap(rgb, i, tree->numemptyareas);
			fprintf(fp, "color %f %f %f 1.0\n", rgb[0], rgb[1], rgb[2]);

			fprintf(fp, "triangles\n");
			fprintf(fp, "%i\n", s->numvertices / 3);
			for (int i = 0; i < s->numvertices; i++)
				fprintf(fp, "%f %f %f\n",
					s->vertices[i][0],
					s->vertices[i][1],
					s->vertices[i][2]);
		}
	}

	FileClose(fp);
}

// push every map face into the tree and store in the areas
void BuildSurfaces(bsptree_t *tree)
{
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// fixme: need a method to filter only structural polygons
		if (f->areahint)
			continue;

		polygon_t *polygon = Polygon_Copy(f->polygon);

		FilterPolygonIntoLeaf(tree->root, polygon);
	}

	PrintSurfaces(tree);

	DebugDumpAreaSurfaces(tree);
}


