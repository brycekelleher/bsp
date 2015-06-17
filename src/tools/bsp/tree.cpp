#include "bsp.h"

// takes as input a list of polygons
// Choose a clipping plane (axial aligned, minimise splits)
// divide the list
// recurse on the list until no polygons remain


const float planeepsilon = 0.02f;

typedef struct bsppoly_s
{
	struct bsppoly_s	*next;
	polygon_t		*polygon;

} bsppoly_t;

// global list of bsp nodes
static bspnode_t	*bspnodes;

static bspnode_t *AllocNode()
{
	bspnode_t *n;
	
	n = (bspnode_t*)MallocZeroed(sizeof(bspnode_t));
	
	// link the node into the global list
	n->next = bspnodes;
	bspnodes = n;
	
	return n;
}

static bsptree_t *MallocTree()
{
	return (bsptree_t*)MallocZeroed(sizeof(bsptree_t));
}

static bsppoly_t *MallocBSPPoly(polygon_t *polygon)
{
	bsppoly_t	*p;
	
	p = (bsppoly_t*)MallocZeroed(sizeof(bsppoly_t));
	p->polygon = polygon;
	
	return p;
}

static bspnode_t *MallocBSPNode(bsptree_t *tree, bspnode_t *parent)
{
	bspnode_t *n = AllocNode();
	
	n->parent = parent;
	n->tree	= tree;
	
	// link the node into the tree list
	n->treenext = tree->nodes;
	tree->nodes = n;
	tree->numnodes++;
	
	return n;
}

static int PolygonOnPlaneSide(bsppoly_t *p, plane_t plane)
{
	vec3 normal	= plane.Normal();
	float distance	= plane.Distance();
	
	return Polygon_OnPlaneSide(p->polygon, normal, distance, planeepsilon);
}

static bool CheckPolygonOnPlane(bsppoly_t *p, plane_t plane)
{
	return (PolygonOnPlaneSide(p, plane) == PLANE_SIDE_ON);
}

static plane_t PolygonPlaneFromPoints(bsppoly_t *p)
{
	plane_t plane;
	
	plane.FromPoints(p->polygon->vertices[0], p->polygon->vertices[1], p->polygon->vertices[2]);
	return plane;
}

static plane_t PlaneFromArea(bsppoly_t *p)
{
	polygon_t *polygon = p->polygon;

	vec3 area = vec3(0, 0, 0);
	for(int i = 0; i < polygon->numvertices; i++)
	{
		int j = (i + 1) % polygon->numvertices;
		
		vec3 vi = polygon->vertices[i];
		vec3 vj = polygon->vertices[j];
		
		area[2] += (vi[0] * vj[1]) - (vj[0] * vi[1]);
		area[0] += (vi[1] * vj[2]) - (vj[1] * vi[2]);
		area[1] += (vi[2] * vj[0]) - (vj[2] * vi[0]);
	}
	
	area = Normalize(area);
	
	plane_t plane;
	plane.a = area[0];
	plane.b = area[1];
	plane.c = area[2];
	plane.d = -Dot(polygon->vertices[0], area);
	
	return plane;
}
	
static plane_t PolygonAveragePlane(bsppoly_t *p)
{
	polygon_t *polygon = p->polygon;
	
	vec3 normal = vec3(0, 0, 0);
	float distance = 0.0f;
	int numtriangles = polygon->numvertices - 2;
	for(int i = 0; i < numtriangles; i++)
	{
		vec3 s = polygon->vertices[i + 1] - polygon->vertices[0];
		vec3 t = polygon->vertices[i + 2] - polygon->vertices[i + 1];
		vec3 cross = Cross(s, t);
		
		cross = Normalize(cross);
		normal = normal + cross;	// fixme: cant use += ?
		
		distance += -Dot(cross, polygon->vertices[0]);
	}
	
	normal = (1.0f / numtriangles) * normal;
	distance = (1.0f / numtriangles) * distance;
	
	return plane_t(normal.x, normal.y, normal.z, distance);
}

// fixme: should the polygon class do this?
// fixme: bug: There's a problem with the Polygon_PlaneFromPoints. All of the points must lie
// in the plane otherwise the rescursion may not terminate. For the moment calculate the average normal
// and then check. The 'proper' way might be to use projected areas?
static plane_t PolygonPlane(bsppoly_t *p)
{
	plane_t plane;
	
	plane = PolygonAveragePlane(p);
	//plane_t plane2  = PlaneFromArea(p);
	
	plane  = PlaneFromArea(p);
	
	if(!CheckPolygonOnPlane(p, plane))
		Error("Polygon doesn't lie on calculated plane\n");
	
	return plane;
}


static void SplitPolygon(plane_t plane, bsppoly_t *p, bsppoly_t **f, bsppoly_t **b)
{
	polygon_t *ff, *bb;

	*f = *b = NULL;
	
	// split the polygon
	{
		vec3 normal	= plane.Normal();
		float distance	= plane.Distance();
		Polygon_SplitWithPlane(p->polygon, normal, distance, planeepsilon, &ff, &bb);
	}
	
	if(ff)
	{
		*f = MallocBSPPoly(ff);
	
		// fixme: probably a better way to do this?
		plane_t inplane = PolygonPlane(p);
		if(!CheckPolygonOnPlane(*f, inplane))
			Error("Front polygon doesn't sit on original plane after split\n");
	}
	
	if(bb)
	{
		*b = MallocBSPPoly(bb);

		// fixme: probably a better way to do this?
		plane_t inplane = PolygonPlane(p);
		if(!CheckPolygonOnPlane(*b, inplane))
			Error("Back polygon doesn't sit on original plane after split\n");
	}
}



// ==============================================
// List processing

// this reverses the order of the list...
// could implement it as a queue. Or take the recursion hit of (cons node (MakePolygonList(node->next)))
static bsppoly_t *MakePolygonList()
{
	bsppoly_t	*list = NULL;
	
	for(mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// allocate a new bsppoly
		bsppoly_t *p	= MallocBSPPoly(f->polygon);
		
		// link it into the list
		p->next = list;
		list = p;
	}
	
	return list;
}

static void SplitPolygonList(plane_t plane, bsppoly_t *list, bsppoly_t **sides)
{
	sides[0] = NULL;
	sides[1] = NULL;
	
	for(; list; list = list->next)
	{
		bsppoly_t *split[2];
		int i;

		SplitPolygon(plane, list, &split[0], &split[1]);

		// process the front (0) and back (1) splits
		for(i = 0; i < 2; i++)
		{
			if(split[i])
			{
				split[i]->next = sides[i];
				sides[i] = split[i];
			}
		}
	}
}

// ==============================================
// Tree building code

static bsptree_t *MakeEmptyTree()
{
	bsptree_t	*tree;
	
	tree = MallocTree();
	tree->root = MallocBSPNode(tree, NULL);

	return tree;
}


static int CalculateSplitPlaneScore(plane_t plane, bsppoly_t *list)
{
	int score = 0;
	
	for(; list; list = list->next)
	{
		// favour polygons that don't cause splits
		int side = PolygonOnPlaneSide(list, plane);
		if(side != POLYGON_SIDE_ON && side != POLYGON_SIDE_CROSS)
			score += 1;
		
		// favour planes which are axial
		if(plane.IsAxial())
			score += 1;
	}
	
	return score;
}


static plane_t ChooseBestSplitPlane(bsppoly_t *list)
{
	int bestscore = 0;
	plane_t bestplane;
	bsppoly_t *p;
	
	for(p = list; p; p = p->next)
	{
		plane_t plane = PolygonPlane(list);
		int score = CalculateSplitPlaneScore(plane, list);
	
		if(!bestscore || score > bestscore)
		{
			bestscore	= score;
			bestplane	= plane;
		}
			
	}
	
	return bestplane;
}

// this will be called with a node and a list. The list will be of polygons fully
// contained within the node. Then the node is split with a plane
// Leaf nodes won't have a valid split plane
// Leaf nodes wont have valid child pointers
static void BuildTreeRecursive(bsptree_t *tree, bspnode_t *node, bsppoly_t *list, int depth)
{
	plane_t		plane;
	bsppoly_t	*sides[2];
	
	// update the depth
	if(depth > tree->depth)
		tree->depth = depth;
	
	// guard condition for an empty list
	if(!list)
	{
		// link node into the leaf list
		node->leafnext = tree->leafs;
		tree->leafs = node;
		
		tree->numleafs++;
		return;
	}
	
	// choose the best split plane for the list
	plane = ChooseBestSplitPlane(list);

	// split the polygon list
	SplitPolygonList(plane, list, sides);

	node->plane = plane;
	
	// add two new nodes to the tree
	node->children[0] = MallocBSPNode(tree, node);
	node->children[1] = MallocBSPNode(tree, node);
	
	// recurse down the front and back sides
	BuildTreeRecursive(tree, node->children[0], sides[0], depth + 1);
	BuildTreeRecursive(tree, node->children[1], sides[1], depth + 1);
}

bsptree_t *BuildTree()
{
	bsptree_t	*tree;
	bsppoly_t	*list;

	list = MakePolygonList();

	tree = MakeEmptyTree();
	
	BuildTreeRecursive(tree, tree->root, list, 1);
	
	return tree;
}

