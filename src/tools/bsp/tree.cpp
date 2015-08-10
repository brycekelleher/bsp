#include "bsp.h"

// takes as input a list of polygons
// Choose a clipping plane (axial aligned, minimise splits)
// divide the list
// recurse on the list until no polygons remain

typedef struct bsppoly_s
{
	struct bsppoly_s	*next;
	polygon_t		*polygon;
	plane_t			plane;
	box3			box;
	bool			areahint;

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
	
	n->empty = false;
	
	return n;
}

static int PolygonOnPlaneSide(bsppoly_t *p, plane_t plane)
{
	return Polygon_OnPlaneSide(p->polygon, plane, CLIP_EPSILON);
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

// ==============================================
// tree walking functions
// bsp callback function for walking the tree

typedef void (*bspcallback_t)(bspnode_t *n);

static void Walk(bspnode_t *n, bspcallback_t callback)
{
	if (!n)
	{
		return;
	}
	
	callback(n);
	Walk(n->children[0], callback);
	Walk(n->children[1], callback);
}

static void WalkWithBox(bspnode_t *n, box3 box, bspcallback_t callback)
{
	// if this is a leaf then callback
	if(!n->children[0] && !n->children[1])
	{
		callback(n);
		return;
	}
	
	int side = n->plane.BoxOnPlaneSide(box, CLIP_EPSILON);
	
	if(side == PLANE_SIDE_FRONT)
		WalkWithBox(n->children[0], box, callback);
	else if (side == PLANE_SIDE_BACK)
		WalkWithBox(n->children[1], box, callback);
	else if(side == PLANE_SIDE_CROSS)
	{
		WalkWithBox(n->children[0], box, callback);
		WalkWithBox(n->children[1], box, callback);
	}
}

static bspnode_t *WalkWithPoint(bspnode_t *n, vec3 p)
{
	// if this is a leaf then return it
	if(!n->children[0] && !n->children[1])
	{
		return n;
	}
	
	int side = n->plane.PointOnPlaneSide(p, CLIP_EPSILON);
	
	if (side == PLANE_SIDE_FRONT || side == PLANE_SIDE_ON)
		return WalkWithPoint(n->children[0], p);
	else
		return WalkWithPoint(n->children[1], p);
}


// ==============================================
// Tree building code

static void SplitPolygon(bsppoly_t *p, plane_t plane, float epsilon, bsppoly_t **f, bsppoly_t **b)
{
	polygon_t *ff, *bb;
	
	*f = *b = NULL;
	
	// split the polygon
	Polygon_SplitWithPlane(p->polygon, plane, CLIP_EPSILON, &ff, &bb);
	
	if (ff)
	{
		*f = MallocBSPPoly(ff);
		(*f)->plane	= p->plane;
		(*f)->box	= p->box;
		(*f)->areahint	= p->areahint;
	}
	if (bb)
	{
		*b = MallocBSPPoly(bb);
		(*b)->plane	= p->plane;
		(*b)->box	= p->box;
		(*b)->areahint	= p->areahint;
	}
	
	// check that the split polygon sits in the original's plane
	if (*f && !CheckPolygonOnPlane(*f, PolygonPlane(p)))
		Error("Front polygon doesn't sit on original plane after split\n");
	if (*b && !CheckPolygonOnPlane(*b, PolygonPlane(p)))
		Error("Back polygon doesn't sit on original plane after split\n");
	
	if(ff && ff->numvertices == 3)
		printf("");
	if(bb && bb->numvertices == 3)
		printf("");
}

// this reverses the order of the list...
// could implement it as a queue. Or take the recursion hit of (cons node (MakePolygonList(node->next)))
static bsppoly_t *MakePolygonList()
{
	bsppoly_t	*list = NULL;
	
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// allocate a new bsppoly
		bsppoly_t *p	= MallocBSPPoly(f->polygon);
		p->plane	= f->plane;
		p->box		= f->box;
		p->areahint	= f->areahint;
		
		// link it into the list
		p->next = list;
		list = p;
	}
	
	return list;
}

static box3 CalculatePolygonListBoundingBox(bsppoly_t *list)
{
	box3 box;
	
	for (; list; list = list->next)
		box.AddBox(list->box);
	
	return box;
}

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
	
	// favour planes which are axial
	if(plane.IsAxial())
		score += 50;
	
	for(; list; list = list->next)
	{
		// favour polygons that don't cause splits
		int side = PolygonOnPlaneSide(list, plane);
		if(side == PLANE_SIDE_CROSS)
			score += -5;
	}
	
	return score;
}

static plane_t ChooseBestSplitPlane(bsppoly_t *list)
{
	int bestscore = 0;
	plane_t bestplane;
	bsppoly_t *p;
	
	// check the node area
	// fixme:

#if 1
	// check the area portals
	{
		bestscore = -9999999;
		for (p = list; p; p = p->next)
		{
			// filter everything that is not an areahint
			if (!p->areahint)
				continue;

			plane_t plane = PolygonPlane(p);
			int score = CalculateSplitPlaneScore(plane, list);
		
			if(score > bestscore)
			{
				bestscore	= score;
				bestplane	= plane;
			}
		}

		if(bestscore != -9999999)
			return bestplane;
	}
#endif
	
	// check the structural polygons
	{
		bestscore = -9999999;
		for (p = list; p; p = p->next)
		{
			// filter areahint polygons
			if (p->areahint)
				continue;

			plane_t plane = PolygonPlane(p);
			int score = CalculateSplitPlaneScore(plane, list);
		
			if(score > bestscore)
			{
				bestscore	= score;
				bestplane	= plane;
			}
				
		}

		return bestplane;
	}
}


static void PartitionPolygonList(plane_t plane, bsppoly_t *list, bsppoly_t **sides)
{
	sides[0] = NULL;
	sides[1] = NULL;
	
	for(; list; list = list->next)
	{
		bsppoly_t *split[2];
		int i;
		
		SplitPolygon(list, plane, CLIP_EPSILON, &split[0], &split[1]);
		
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

box3 ClipNodeBoxWithPlane(box3 box, plane_t plane)
{
	box3 clipped;

	for(int i = 0; i < 3; i++)
	{
		if (plane[i] == 1)
		{
			// plane points in positive direction so clip min
			clipped.min[i] = -plane[3];
			clipped.max[i] = box.max[i];
		}
		else if (plane[i] == -1)
		{
			// plane points in negative direction so clip max 
			clipped.min[i] = box.min[i];
			clipped.max[i] = plane[3];
		}
		else
		{
			// copy the original bound through for this axis
			clipped.min[i] = box.min[i];
			clipped.max[i] = box.max[i];
		}
	}

	return clipped;
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
	if (depth > tree->depth)
		tree->depth = depth;
	
	// guard condition for an empty list
	if (!list)
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
	PartitionPolygonList(plane, list, sides);

	node->plane = plane;
	
	// add two new nodes to the tree
	node->children[0] = MallocBSPNode(tree, node);
	node->children[1] = MallocBSPNode(tree, node);
	
	node->children[0]->box = ClipNodeBoxWithPlane(node->box,  node->plane);
	node->children[1]->box = ClipNodeBoxWithPlane(node->box, -node->plane);
	
	// recurse down the front and back sides
	BuildTreeRecursive(tree, node->children[0], sides[0], depth + 1);
	BuildTreeRecursive(tree, node->children[1], sides[1], depth + 1);
}

bsptree_t *BuildTree()
{
	bsptree_t	*tree;
	bsppoly_t	*list;
	box3		box;

	list = MakePolygonList();
	
	tree = MakeEmptyTree();
	
	// setup the root node bounding box
	{
		box = CalculatePolygonListBoundingBox(list);
		box.Expand(8.0f);
		tree->root->box = box;
	}
	
	BuildTreeRecursive(tree, tree->root, list, 1);
	
	return tree;
}


