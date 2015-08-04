#include "bsp.h"

static portal_t *globalportals;

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
	numplanes = 0;
	PlanesForLeafRecursive(node->parent, node);
	
	// add the fictional 'world' planes
	// this won't work as the planes don't have a leaf to fall into
	//planes[numplanes++] = plane_t( 1,  0,  0, 512.0f);	// -x
	//planes[numplanes++] = plane_t(-1,  0,  0, 512.0f);	// +x
	//planes[numplanes++] = plane_t( 0,  1,  0, 512.0f);	// -y
	//planes[numplanes++] = plane_t( 1, -1,  0, 512.0f);	// +y
	//planes[numplanes++] = plane_t( 0,  0,  1, 512.0f);	// -z
	//planes[numplanes++] = plane_t( 1,  0, -1, 512.0f);	// +z
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
	vec3 normal = plane.GetNormal();
	int index = AbsLargestComponent(normal);
	if(normal[index] < 0)
		index += 3;
	vec3 up = vec3(basistab[index][1][0], basistab[index][1][1], basistab[index][1][2]);
	
	u = Cross(up, normal);
	v = Cross(normal, u);
	
	u = Normalize(u);
	v = Normalize(v);
	
	polygon_t *p = Polygon_Alloc(4);
	
	// determine the u and v vectors
	vec3 xyz = plane.GetNormal() * -plane.GetDistance();

	p->vertices[0] = xyz - (usize * u) - (vsize * v);
	p->vertices[1] = xyz + (usize * u) - (vsize * v);
	p->vertices[2] = xyz + (usize * u) + (vsize * v);
	p->vertices[3] = xyz - (usize * u) + (vsize * v);
	p->numvertices = 4;
	
	return p;
}

static FILE *portalfp;

static FILE *OpenPortalFile(bspnode_t *l)
{
	static char filename[2048];
	sprintf(filename, "portal_%p.gl", l);

	return FileOpenTextWrite(filename);
}

static void AddPortalToLeaf(bspnode_t *srcleaf, polygon_t *polygon, bspnode_t *dstleaf)
{
	portal_t *portal;
	
	portal = (portal_t*)MallocZeroed(sizeof(portal_t));
	
	portal->srcleaf = srcleaf;
	portal->dstleaf = dstleaf;
	portal->polygon = polygon;
	
	// link it into the leaflist
	portal->leafnext = srcleaf->portals;
	srcleaf->portals = portal;
	
	// link into the global list
	portal->globalnext = globalportals;
	globalportals = portal;
}

// walk up the tree from leaf to root and clip the polygon in place
// it's possible for an entire polygon to get clipped away if the plane
// doesn't form the one of the immediate 'bounding planes' - ie it's
// further up in the tree. If this happens we need to bail out
static polygon_t *ClipPolygonAgainstLeafIterative(polygon_t *p, bspnode_t *leaf)
{
	bspnode_t *prev = leaf;
	bspnode_t *node = leaf->parent;
	
	while (node && p)
	{
		// have to special case this as side on will clip the polygon
		if (Polygon_OnPlaneSide(p, node->plane, epsilon) == PLANE_SIDE_ON)
			continue;
		
		// flip the plane if the previous node was on the back side
		plane_t plane = node->plane;
		if (node->children[1] == prev)
			plane = -plane;
		
		// clip the polygon with the current leaf plane
		p = Polygon_ClipWithPlane(p, plane, epsilon);
		
		// walk another level up the tree
		prev = node;
		node = node->parent;
	}
	
	return p;
}

// wind up to the root then clip on unwind
static polygon_t *ClipPolygonAgainstLeafRecursive(polygon_t *p, bspnode_t *node, bspnode_t *prev)
{
	if (!node)
		return p;
	
	p = ClipPolygonAgainstLeafRecursive(p, node->parent, node);
	
	// the result of the previous clip might have completely clipped the polygon
	if (!p)
		return NULL;
	
	// have to special case this as side on will clip the polygon
	if (Polygon_OnPlaneSide(p, node->plane, epsilon) == PLANE_SIDE_ON)
		return p;
	
	// flip the plane if we followed the back link to get here
	plane_t plane = node->plane;
	if (node->children[1] == prev)
		plane = -plane;
	
	// clip the polygon with the current leaf plane
	return Polygon_ClipWithPlane(p, plane, epsilon);
	
	return p;
}

static polygon_t *ClipPolygonAgainstLeaf(polygon_t *p, bspnode_t *leaf)
{
	return ClipPolygonAgainstLeafRecursive(p, leaf, NULL);
}

polygon_t* BuildLeafPolygon(plane_t plane)
{
	polygon_t * p = PolygonForPlane(plane, 512.0f, 512.0f);
	
	return p;
}

void PushPotalPolygonIntoLeafs(bspnode_t *node, polygon_t *polygon, bspnode_t *srcleaf)
{
	if (!node->children[0] && !node->children[1])
	{
		// it's not a portal if it's landed back in the original leaf
		if (node == srcleaf)
			return;
		
		// this portal has landed in a leaf node that's not the leaf the source portal came from
		// this means a connection exists from srcleaf to this node
		AddPortalToLeaf(srcleaf, polygon, node);
		
		DebugWriteWireFillPolygon(portalfp, polygon);
		return;
	}
	
	int side = Polygon_OnPlaneSide(polygon, node->plane, epsilon);
	
	if (side == PLANE_SIDE_FRONT)
	{
		PushPotalPolygonIntoLeafs(node->children[0], polygon, srcleaf);
	}
	else if (side == PLANE_SIDE_BACK)
	{
		PushPotalPolygonIntoLeafs(node->children[1], polygon, srcleaf);
	}
	else if (side == PLANE_SIDE_ON)
	{
		polygon_t *f, *b;
		f = Polygon_Copy(polygon);
		b = Polygon_Copy(polygon);
		PushPotalPolygonIntoLeafs(node->children[0], f, srcleaf);
		PushPotalPolygonIntoLeafs(node->children[1], b, srcleaf);
	}
	else if (side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(polygon, node->plane, epsilon, &f, &b);
		PushPotalPolygonIntoLeafs(node->children[0], f, srcleaf);
		PushPotalPolygonIntoLeafs(node->children[1], b, srcleaf);
	}
}

// for each plane in the convex volume
	// create a polygons on each of the convex volume sides
	// push this polygon into the tree and see what leaves it
	// falls into

// It may also work to build a polygon for plane of the leaf, then clip it against every other leaf
// if a polygon remains after clipping into a plane, then a portal must exist on from that leaf
// to the other
static void ProcessLeaf(bspnode_t *root, bspnode_t *leaf)
{
	portalfp = OpenPortalFile(leaf);
	
	//Message("Process leaf %p\n", l);
	
	// grab the planes which form the leaf
	PlanesForLeaf(leaf);
	
	// for each leaf face
	for(int i = 0; i < numplanes; i++)
	{
		// create a polygon for the leaf plane
		polygon_t *polygon = BuildLeafPolygon(planes[i]);
		
		// clip the polygon against the leaf
		polygon = ClipPolygonAgainstLeaf(polygon, leaf);
		
		if (!polygon)
			continue;
		
		// push it into the tree
		// see which leaf it pops into
		//ProcessLeafPolygon(l, polygon);
		PushPotalPolygonIntoLeafs(root, polygon, leaf);
	}
	
	FileClose(portalfp);
}

void FindPortals(bsptree_t *tree)
{
	Message("Portalizing tree\n");
	
	for(bspnode_t *l = tree->leafs; l; l = l->leafnext)
		ProcessLeaf(tree->root, l);
	
	DebugWritePortalFile(tree);
}

