#include "bsp.h"

areatri_t *AllocAreaTri()
{
	areatri_t *n = (areatri_t*)MallocZeroed(sizeof(areatri_t));
	return n;
}

areatri_t *Copy(areatri_t *t)
{
	areatri_t *c = AllocAreaTri();
	*c = *t;
	c->next = NULL;
	return c;
}

trilist_t *CreateTriList()
{
	trilist_t *l = (trilist_t*)MallocZeroed(sizeof(trilist_t));
	return l;
}

void FreeTriList(trilist_t *l)
{
	areatri_t *next;
	while(l->head)
	{
		next = l->head->next;
		// FreeTriangle
		l->head = next;
	}

	free(l);
}

void Append(trilist_t *l, areatri_t *t)
{
	// empty list special case
	if (!l->head && !l->tail)
	{
		l->head = l->tail = t;
		return;
	}

	l->tail->next = t;
	l->tail = t;
}

areatri_t *Head(trilist_t *l)
{
	return l->head;
}

int Length(trilist_t *l)
{
	int count = 0;
	for(areatri_t *t = l->head; t; t = t->next)
		count++;
	return count;
}

void CheckTriArea(areatri_t *t)
{
	vec3 area = vec3(0, 0, 0);

	// for each edge of the triangle
	for (int i = 0; i < 3; i++)
	{
		int i0 = (i + 0) % 3;
		int i1 = (i + 1) % 3;
				
		vec3 v0 = t->vertices[i0];
		vec3 v1 = t->vertices[i1];
				
		area[2] += (v0[0] * v1[1]) - (v1[0] * v0[1]);
		area[0] += (v0[1] * v1[2]) - (v1[1] * v0[2]);
		area[1] += (v0[2] * v1[0]) - (v1[2] * v0[0]);
	}

	if (area[0] == 0.0f && area[1] == 0.0f & area[2] == 0.0f)
		Error("Zero area tri!\n");
}

// ==============================================
// Surface code

#if 0
static trisurf_t *TS_New()
{
	return (trisurf_t*)MallocZeroed(sizeof(trisurf_t));
}

// fixme: reimplement realloc?
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

// fan the polygon out into the trianglelist
static void AddPolygonToArea(bspnode_t *n, polygon_t* p)
{
	area_t *a = n->area;

	// get the surface
	if (!a->trisurf)
		a->trisurf = TS_New();
	trisurf_t *s = a->trisurf;

	for (int i = 0; i < p->numvertices - 2; i++)
		TS_AddTriangle(s, p->vertices[0], p->vertices[i + 1], p->vertices[i + 2]);
}
#endif

// ==============================================
// BSP surface code


#if 0
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
#endif

static void PrintSurfaces(bsptree_t *tree)
{
	for (area_t *a = tree->areas; a; a = a->next)
	{
		trisurf_t *s = a->trisurf;

		if (!s)
			continue;

		printf("area %p surface\n", a);
		for (int i = 0; i < s->numvertices; i++)
			printf("vertex %i: (%f %f %f)\n", i, s->vertices[i][0], s->vertices[i][1], s->vertices[i][2]);
	}
}

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

// builds lists of polygon fragments in each area
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

// push every map face into the tree and store in the areas
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
// Tjunctions

/*
def classify_vertex_against_edge(vertex, v0, v1):
	cl = "d"

	dv0 = length3(vertex - v0)
	dv1 = length3(vertex - v1)
	d, t, p = edgetest(v0, v1, vertex)

	if d < 0.01 and t >= 0.0 and t <= 1.0:
		cl = "e"
	# vertex test eats the edge test
	if dv0 < 0.00001 or dv1 < 0.000001:
		cl = "v"

	return cl
*/

/*
def edgetest(v0, v1, vertex):
	n = v1 - v0
	n = (1.0 / (length3(n) + 0.000001)) * n

	# p is the point on the line that is perp with vertex
	p = (dot3((vertex - v0), n) * n) + v0

	d = length3(vertex - p)
	t = dot3((vertex - v0), n) / (length3(v1 - v0) + 0.0000001)
	return (d, t, p)
*/

typedef struct tjunc_edgetest_s
{
	// the distance, t parameter and actual point on the edge
	float	d;
	float	t;
	vec3	p;

} tjunc_edgetest_t;

// fixme: could use normalize function?
static tjunc_edgetest_t EdgeTest(vec3 v, vec3 ev0, vec3 ev1)
{
	vec3 e, p;
	float d, t;

	// calculate normalized edge vector
	e = ev1 - ev0;
	e = (1.0 / (Length(e) + 0.000001)) * e;

	// p is the point on the line that is perp with vertex
	p = (Dot((v - ev0), e) * e) + ev0;

	d = Length(v - p);
	t = Dot((v - ev0), e) / (Length(ev1 - ev0) + 0.0000001);

	tjunc_edgetest_t result = { d, t, p };
	return result;
}

enum tjunc_vcl_t
{
	TJUNC_DISJOINT,
	TJUNC_VERTEX,
	TJUNC_EDGE	
};

// fixme: need edge_tolerance and vertex_tolerance constants
static tjunc_vcl_t ClassifyVertexAgainstEdge(vec3 v, vec3 ev0, vec3 ev1)
{
	tjunc_vcl_t cl = TJUNC_DISJOINT;

	float dv0 = Length(v - ev0);
	float dv1 = Length(v - ev1);
	tjunc_edgetest_t edgetest = EdgeTest(v, ev0, ev1);

	if ((edgetest.d < 0.01f) && (edgetest.t >= 0.0) && (edgetest.t <= 1.0))
		cl = TJUNC_EDGE;

	// vertex test eats the edge test
	//printf("v: %f %f %f, ev0: %f %f %f, ev1: %f %f %f\n", v[0], v[1], v[2], ev0[0], ev0[1], ev0[2], ev1[0], ev1[1], ev1[2]);
	//printf("dv0: %f, dv1: %f\n", dv0, dv1);
	if ((dv0 < 0.1f) || (dv1 < 0.1f))
		cl = TJUNC_VERTEX;

	return cl;
}

static trilist_t *FixTriangleList(trilist_t *trilist, vec3 v)
{
	trilist_t *result = CreateTriList();

	for (areatri_t *t = Head(trilist); t; t = t->next)
	{
		tjunc_vcl_t cl0 = ClassifyVertexAgainstEdge(v, t->vertices[0], t->vertices[1]);
		tjunc_vcl_t cl1 = ClassifyVertexAgainstEdge(v, t->vertices[1], t->vertices[2]);
		tjunc_vcl_t cl2 = ClassifyVertexAgainstEdge(v, t->vertices[2], t->vertices[0]);

		if (cl0 == TJUNC_EDGE)
		{
			areatri_t *n;
			Warning("found tjunc cl0\n");

			n = Copy(t);
			n->vertices[0] = t->vertices[0];
			n->vertices[1] = v;
			n->vertices[2] = t->vertices[2];
			CheckTriArea(n);
			Append(result, n);

			n = Copy(t);
			n->vertices[0] = v;
			n->vertices[1] = t->vertices[1];
			n->vertices[2] = t->vertices[2];
			CheckTriArea(n);
			Append(result, n);
		}
		else if (cl1 == TJUNC_EDGE)
		{
			areatri_t *n;
			Warning("found tjunc cl1\n");

			n = Copy(t);
			n->vertices[0] = t->vertices[0];
			n->vertices[1] = t->vertices[1];
			n->vertices[2] = v;
			CheckTriArea(n);
			Append(result, n);

			n = Copy(t);
			n->vertices[0] = t->vertices[0];
			n->vertices[1] = v;
			n->vertices[2] = t->vertices[2];
			CheckTriArea(n);
			Append(result, n);
		}
		else if (cl2 == TJUNC_EDGE)
		{
			areatri_t *n;
			Warning("found tjunc cl2\n");

			n = Copy(t);
			n->vertices[0] = t->vertices[0];
			n->vertices[1] = t->vertices[1];
			n->vertices[2] = v;
			CheckTriArea(n);
			Append(result, n);

			n = Copy(t);
			n->vertices[0] = v;
			n->vertices[1] = t->vertices[1];
			n->vertices[2] = t->vertices[2];
			CheckTriArea(n);
			Append(result, n);
		}
		else
		{
			Append(result, Copy(t));
		}
	}

	return result;
}

static trilist_t *FixTJunctions(trilist_t *trilist)
{
	trilist_t *result = CreateTriList();

	for (areatri_t *t = Head(trilist); t; t = t->next)
	{
		// Make a list of a single element
		trilist_t *fixedlist = CreateTriList();
		Append(fixedlist, Copy(t));

		// iterate through every vertex and test for violations
		for (areatri_t *tv = Head(trilist); tv; tv = tv ->next)
		{
			for (int i = 0; i < 3; i++)
			{
				// update the fixed list
				trilist_t *result = FixTriangleList(fixedlist, tv->vertices[i]);
				FreeTriList(fixedlist);
				fixedlist = result;
			}
		}

		// merge the fixed list with the result
		// fixme: Do we copy the triangle again?
		for (areatri_t *t = Head(fixedlist); t; t = t->next)
			Append(result, Copy(t));
		FreeTriList(fixedlist);
	}

	return result;
}

//________________________________________________________________________________
// Trilist normals

static float min3(float a, float b, float c)
{
	if (a < b && a < c)
		return a;
	else if (b < c)
		return b;
	else
		return c;
}

static vec3 TriNormal(areatri_t *t)
{
	vec3 area = vec3(0, 0, 0);

	//printf("v0 %f %f %f\n", t->vertices[0][0], t->vertices[1][0], t->vertices[2][0]);
	//printf("v1 %f %f %f\n", t->vertices[0][1], t->vertices[1][1], t->vertices[2][1]);
	//printf("v2 %f %f %f\n", t->vertices[0][2], t->vertices[1][2], t->vertices[2][2]);

	// for each edge of the triangle
	for (int i = 0; i < 3; i++)
	{
		int i0 = (i + 0) % 3;
		int i1 = (i + 1) % 3;
				
		vec3 v0 = t->vertices[i0];
		vec3 v1 = t->vertices[i1];
				
		area[2] += (v0[0] * v1[1]) - (v1[0] * v0[1]);
		area[0] += (v0[1] * v1[2]) - (v1[1] * v0[2]);
		area[1] += (v0[2] * v1[0]) - (v1[2] * v0[0]);
	}
		
	//printf("area %f %f %f\n", area[0], area[1], area[2]);
	if (Length(area) <= 0.00001f)
		return vec3(0, 0, 0);
	return Normalize(area);
}

static bool DistanceCheck(vec3 a, vec3 b, float tolerance = 0.00001f)
{
	return Length(a - b) <= tolerance;
}

static bool SearchNormalList(vec3 *list, int numitems, vec3 value)
{
	int i = 0;
	for (; i < numitems; i++)
		if (list[i] == value)
			break;
	return (i == numitems);
}

static vec3 CalculateVertexNormal(vec3 vertex, vec3 snormal, trilist_t *trilist)
{
	vec3 normallist[256];
	int numnormals = 0;

	// build the normal list
	for (areatri_t *t = trilist->head; t; t = t->next)
	{
		vec3 n = TriNormal(t);

		if (n[0] == 0 && n[1] == 0 && n[2] == 0)
		{
			Warning("zero area normal found\n");
			continue;
		}

		// distance check
		float l0 = Length(vertex - t->vertices[0]);
		float l1 = Length(vertex - t->vertices[1]);
		float l2 = Length(vertex - t->vertices[2]);
		if (min3(l0, l1, l2) > 0.00001f)
			continue;
		
		// planar check
		if (Dot(snormal, n) < 0.9f)
			continue;

		// search for the normal in the normal list
		int i = 0;
		for (; i < numnormals; i++)
			if (normallist[i] == n)
				break;

		if (i != numnormals)
			continue;

		// overflow check
		if (i == 256)
		{
			Warning("No more normals");
			continue;
		}

		normallist[numnormals++] = n;
	}

	// calculate the average normal
	// fixme: += doesn't work
	vec3 vertexnormal = vec3(0, 0, 0);
	for (int i = 0; i < numnormals; i++)
		vertexnormal = vertexnormal + normallist[i];
	vertexnormal = Normalize(vertexnormal);

	return vertexnormal;
}

static trilist_t *CalculateNormals(trilist_t *trilist)
{
	for (areatri_t *t = trilist->head; t; t = t->next)
	{
		vec3 snormal = TriNormal(t);
		//printf("snormal %f %f %f\n", snormal[0], snormal[1], snormal[2]);
		t->normals[0] = CalculateVertexNormal(t->vertices[0], snormal, trilist);
		t->normals[1] = CalculateVertexNormal(t->vertices[1], snormal, trilist);
		t->normals[2] = CalculateVertexNormal(t->vertices[2], snormal, trilist);
	}

	return trilist;
}

//________________________________________________________________________________

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

	//PrintLeafFaces(tree);
	//WriteGLViewFaces(tree);
	//EmitAreaGeometry(tree);
}

