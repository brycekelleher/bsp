#include "bsp.h"

// walk to the root
void WalkToRoot(bspnode_t *node, bspnode_t *prev)
{
	if(!node)
	{
		return;
	}
	
	WalkToRoot(node->parent, node);
}

void WalkToRoot2(bspnode_t *node, bspnode_t *prev)
{
	if(!node)
	{
		return;
	}

	plane_t plane = node->plane;
	if(node->children[1] == prev)
		plane = -plane;
	
	WalkToRoot2(node->parent, node);
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

