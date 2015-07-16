#include "bsp.h"

// fixme: this should be a constant
static const float epsilon = 0.02f;

// walk to the root
void WalkToRoot(bspnode_t *node, bspnode_t *prev)
{
	if (!node)
		return;
	
	WalkToRoot(node->parent, node);
}

void WalkToRoot2(bspnode_t *node, bspnode_t *prev)
{
	if (!node)
		return;

	plane_t plane = node->plane;
	if (node->children[1] == prev)
		plane = -plane;
	
	WalkToRoot2(node->parent, node);
}

// walk to the root from a leaf node
static plane_t planes[256];
int numplanes = 0;
void PlanesForLeafRecursive(bspnode_t *node, bspnode_t *prev)
{
	if (!node)
		return;
	
	// flip the plane if the previous node was on the back side
	plane_t plane = node->plane;
	if (node->children[1] == prev)
		plane = -plane;
	
	planes[numplanes++] = plane;
	
	PlanesForLeafRecursive(node->parent, node);
}

void PlanesForLeaf(bspnode_t *node)
{
	PlanesForLeafRecursive(node->parent, node);
}

// fixme: this should go into into the vec3 class
int AbsLargestComponent(vec3 v)
{
	float x = fabs(v.x);
	float y = fabs(v.y);
	float z = fabs(v.z);
	
	if(x > y && x > z)
		return 0;
	else if(y > z)
		return 1;
	else
		return 2;
}

static float basistab[6][3][3] =
{
	{ { 0,  0, -1}, { 0,  1,  0}, { 1,  0,  0} },
	{ { 1,  0,  0}, { 0,  0, -1}, { 0,  1,  0} },
	{ { 1,  0,  0}, { 0,  1,  0}, { 0,  0,  1} },
	{ { 0,  0,  1}, { 0,  1,  0}, {-1,  0,  0} },
	{ { 1,  0,  0}, { 0,  0,  1}, { 0, -1,  0} },
	{ {-1,  0,  0}, { 0,  1,  0}, { 0,  0, -1} }
};

// construct a large polygon along the plane
polygon_t *PolygonForPlane(plane_t plane, float usize, float vsize)
{
	vec3	u, v;
	
	// closest axis aligned basis for normal
	vec3 normal = plane.Normal();
	int index = AbsLargestComponent(normal);
	if(normal[index] < 0)
		index *= 2;
	vec3 up = vec3(basistab[index][1][0], basistab[index][1][1], basistab[index][1][2]);
	
	u = Cross(up, normal);
	v = Cross(normal, u);
	
	u = Normalize(u);
	v = Normalize(v);
	
	polygon_t *p = Polygon_Alloc(4);
	
	// determine the u and v vectors
	vec3 xyz = plane.Normal() * -plane.Distance();

	p->vertices[0] = xyz - (usize * u) - (vsize * v);
	p->vertices[1] = xyz + (usize * u) - (vsize * v);
	p->vertices[2] = xyz + (usize * u) + (vsize * v);
	p->vertices[3] = xyz - (usize * u) + (vsize * v);
	p->numvertices = 4.0f;
	
	return p;
}

void TestPolygonForPlane()
{
	plane_t plane;
	
	vec3 n = Normalize(vec3(0.9f, 0.1f, 0.0f));
	plane.a = n[0];
	plane.b = n[1];
	plane.c = n[2];
	plane.d = 0;

	PolygonForPlane(plane, 4096.0f, 4096.0f);
}

void Blah(bspnode_t *n, polygon_t *p)
{
	if(!n->children[0] && !n->children[1])
	{
		// this is a leaf node
		DebugPrintfPolygon(p);
		return;
	}
	
	int side = Polygon_OnPlaneSide(p, n->plane, epsilon);
	
	if(side == PLANE_SIDE_FRONT)
	{
		Blah(n->children[0], p);
	}
	else if(side == PLANE_SIDE_BACK)
	{
		Blah(n->children[1], p);
	}
	else if(side == PLANE_SIDE_ON)
	{
		polygon_t *f, *b;
		f = Polygon_Copy(p);
		b = Polygon_Copy(p);
		Blah(n->children[0], f);
		Blah(n->children[1], b);
	}
	else if(side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(p, n->plane, epsilon, &f, &b);
		Blah(n->children[0], f);
		Blah(n->children[1], b);
	}
}

polygon_t* BuildLeafPolygon(bspnode_t *l, plane_t plane)
{
	polygon_t * p = PolygonForPlane(plane, 4096.0f, 4096.0f);
	
	// walk up the tree from leaf to root and clip the polygon in place
	{
		bspnode_t *prev = l;
		bspnode_t *node = l->parent;

		while(node)
		{
			// flip the plane if the previous node was on the back side
			plane_t plane = node->plane;
			if (node->children[1] == prev)
				plane = -plane;
	
			// have to special case this as side on will clip the polygon
			if (Polygon_OnPlaneSide(p, plane, epsilon) != PLANE_SIDE_ON)
			{
				polygon_t *f, *b;
				
				// clip the polygon
				Polygon_SplitWithPlane(p, plane, epsilon, &f, &b);
				Polygon_Free(p);
				if (b)
					Polygon_Free(b);
				p = f;
			}
	
			// walk another level up the tree
			prev = node;
			node = node->parent;
		}
	}
	
	return p;
}

void ProcessLeaf(bspnode_t *root, bspnode_t *l)
{
	// grab the planes which form the leaf
	PlanesForLeaf(l);
	
	// for each leaf face
	for(int i = 0; i < numplanes; i++)
	{
		// create a polygon
		polygon_t *p = BuildLeafPolygon(l, planes[i]);
		
		// push it into the tree
		// see which leaf it pops into
		//ProcessLeafPolygon(l, polygon);
		Blah(root, p);
	}
}
