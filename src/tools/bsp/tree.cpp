#include "bsp.h"

// takes as input a list of polygons
// Choose a clipping plane (axial aligned, minimise splits)
// divide the list
// recurse on the list until no polygons remain

// at 128 = 1m, 0.02 = 0.15625mm
const float epsilon = 0.02f;

typedef struct bsppoly_s
{
	struct bsppoly_s	*next;
	polygon_t		*polygon;

} bsppoly_t;

// global list of bsp nodes
static bspnode_t	*bspnodes;

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
	bspnode_t *n = (bspnode_t*)MallocZeroed(sizeof(bspnode_t));

	n->parent = parent;
	n->tree	= tree;
	
	// link the node into the global list
	n->globalnext = bspnodes;
	bspnodes = n;
	
	// link the node into the tree list
	n->treenext = tree->nodes;
	tree->nodes = n;
	tree->numnodes++;
	
	return n;
}

static int PolygonOnPlaneSide(bsppoly_t *p, plane_t plane)
{
	return Polygon_OnPlaneSide(p->polygon, plane, epsilon);
}

static bool CheckPolygonOnPlane(bsppoly_t *p, plane_t plane)
{
	return (PolygonOnPlaneSide(p, plane) == PLANE_SIDE_ON);
}

static plane_t PolygonPlane(bsppoly_t *p)
{
	plane_t plane = Polygon_Plane(p->polygon);
	
	if (PolygonOnPlaneSide(p, plane) != PLANE_SIDE_ON)
		Error("Polygon doesn't lie on calculated plane\n");
	
	return plane;
}

static void SplitPolygon(plane_t plane, bsppoly_t *p, bsppoly_t **f, bsppoly_t **b)
{
	polygon_t *ff, *bb;

	*f = *b = NULL;
	
	// split the polygon
	Polygon_SplitWithPlane(p->polygon, plane, epsilon, &ff, &bb);
	
	if (ff)
		*f = MallocBSPPoly(ff);
	if (bb)
		*b = MallocBSPPoly(bb);
	
	// check that the split polygon sits in the original's plane
	if (*f && !CheckPolygonOnPlane(*f, PolygonPlane(p)))
		Error("Front polygon doesn't sit on original plane after split\n");
	if (*b && !CheckPolygonOnPlane(*b, PolygonPlane(p)))
		Error("Back polygon doesn't sit on original plane after split\n");
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
		if(side != PLANE_SIDE_ON && side != PLANE_SIDE_CROSS)
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
		plane_t plane = PolygonPlane(p);
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

