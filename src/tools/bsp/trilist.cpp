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

static void CheckTriArea(areatri_t *t)
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
			Message("found tjunc cl0\n");

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
			Message("found tjunc cl1\n");

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
			Message("found tjunc cl2\n");

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

trilist_t *FixTJunctions(trilist_t *trilist)
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

static vec3 CalculateVertexNormal(vec3 vertex, vec3 refnormal, trilist_t *trilist)
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
		
		// smoothing angle check
		if (Dot(refnormal, n) < 0.9f)
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

trilist_t *CalculateNormals(trilist_t *trilist)
{
	for (areatri_t *t = trilist->head; t; t = t->next)
	{
		vec3 refnormal = TriNormal(t);
		//printf("refnormal %f %f %f\n", refnormal[0], refnormal[1], refnormal[2]);
		t->normals[0] = CalculateVertexNormal(t->vertices[0], refnormal, trilist);
		t->normals[1] = CalculateVertexNormal(t->vertices[1], refnormal, trilist);
		t->normals[2] = CalculateVertexNormal(t->vertices[2], refnormal, trilist);
	}

	return trilist;
}

//________________________________________________________________________________
// Triangle Clipping
// consumes a triangle and a plane and produces a two lists of triangles for the front and back planes

// Clip the triangle with a plane generating a new polygon.
// Triangulate the polygon and create new triangles

void ClipTriangleWithPlane(areatri_t *t, plane_t plane, float epsilon, trilist_t **f,trilist_t **b)
{
	int sides[4];
	int counts[3];
	int i, j;

	counts[0] = counts[1] = counts[2] = 0;

	// compute the sides
	for (int i = 0; i < 3; i++)
	{
		sides[i] = PointOnPlaneSide(plane, t->vertices[i], epsilon);
		counts[sides[i]]++;
	}

	// triangle sits on the plane
	if (!counts[PLANE_SIDE_FRONT] && !counts[PLANE_SIDE_BACK])
	{
		*f = NULL;
		*b = NULL;
		return;	
	}

	// triangle in front of plane
	if (!counts[PLANE_SIDE_BACK])
	{
		*f = CreateTriList();
		Append(*f, Copy(t));
		return;	
	}

	// triangle on back of plane
	if (!counts[PLANE_SIDE_FRONT])
	{
		*b = CreateTriList();
		Append(*b, Copy(t));
		return;	
	}

	// triangle crosses the plane
	// The split triangle
	// 0 is front, 1 is back
	vec3 vertices[2][4];
	int numvertices[2];
	
	numvertices[0] = numvertices[1] = 0;

	for (i = 0; i < 3; i++)
	{
		vec3	p1, p2, mid;

		p1 = t->vertices[i];
		p2 = t->vertices[(i + 1) % 3];
		
		if (sides[i] == PLANE_SIDE_ON)
		{
			// add the point to the front polygon
			vertices[0][numvertices[0]] = p1;
			numvertices[0]++;

			// Add the point to the back polygon
			vertices[1][numvertices[1]] = p1;
			numvertices[1]++;
			
			continue;
		}
	
		if (sides[i] == PLANE_SIDE_FRONT)
		{
			// add the point to the front polygon
			vertices[0][numvertices[0]] = p1;
			numvertices[0]++;
		}

		if (sides[i] == PLANE_SIDE_BACK)
		{
			vertices[1][numvertices[1]] = p1;
			numvertices[1]++;
		}

		// if the next point doesn't straddle the plane continue
		if (sides[i + 1] == PLANE_SIDE_ON || sides[i] == sides[i + 1])
		{
			continue;
		}
		
		// The next point crosses the plane, so generate a split point
		for (j = 0; j < 3; j++)
		{
			// avoid round off error when possible
			if (plane[j] == 1)
			{
				mid[j] = -plane[3];
			}
			else if (plane[j] == -1)
			{
				mid[j] = plane[3];
			}
			else
			{
				float dist1, dist2, dot;
				
				dist1 = Distance(plane, p1);
				dist2 = Distance(plane, p2);
				dot = dist1 / (dist1 - dist2);
				mid[j] = ((1.0f - dot) * p1[j]) + (dot * p2[j]);
			}
		}
			
		vertices[0][numvertices[0]] = mid;
		numvertices[0]++;
		vertices[1][numvertices[1]] = mid;
		numvertices[1]++;
	}

	// build the front triangle list
	for (i = 0; i < numvertices[0]; i++)
	{}

	// build the back triangle list
	for (i = 0; i < numvertices[1]; i++)
	{}
}



