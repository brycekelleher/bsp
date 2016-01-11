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

// A candidate split plane has features f1, f2 .. fn that characterize it
// The split plane's 'static value' is a function of it's features
// Typically this function is a linear combination of the features called a 'linear scoring polynomial'
// the weights are the cofficients of the polynomial
typedef struct plane_features_s
{
	bool	areahint;
	bool	axial;
	int	sides[4];
	float	areas[4];

} plane_features_t;

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

static box3 CalculatePolygonListBoundingBox(bsppoly_t *list)
{
	box3 box;
	
	for (; list; list = list->next)
		box.AddBox(list->box);
	
	return box;
}

static int Length(bsppoly_t *list)
{
	int i = 0;
	for(; list; list = list->next)
		i++;
	
	return i;
}

static plane_features_t ComputeSplitPlaneFeatures(plane_t plane, bool areahint, bsppoly_t *list)
{
	plane_features_t	f;

	f.areahint = areahint;
	f.axial = plane.IsAxial();
	f.sides[0] = 0;
	f.sides[1] = 0;
	f.sides[2] = 0;
	f.sides[3] = 0;
	f.areas[0] = 0.0f;
	f.areas[1] = 0.0f;
	f.areas[2] = 0.0f;
	f.areas[3] = 0.0f;
	// zero_int_array(f.sides, 4)

	for(; list; list = list->next)
	{
		int side = PolygonOnPlaneSide(list, plane);

		f.sides[side]	+= 1;
		f.areas[side]	+= Polygon_Area(list->polygon);
	}

	return f;
}

static float ComputeSplitPlaneScore(plane_t plane, bool areahint, bsppoly_t *list)
{
	plane_features_t f = ComputeSplitPlaneFeatures(plane, areahint, list);

	int listsize = Length(list);

	// compute areahint feature
	float f1 = (f.areahint ? 1.0f : 0.0f);
	float w1 = 1.0f;

	// compute axial feature
	float f2 = (f.axial ? 1.0f : 0.0f);
	float w2 = 1.0f;

	// compute split feature
	float f3 = 1.0f - ((float)f.sides[PLANE_SIDE_CROSS] / (float)listsize);
	float w3 = 1.0f;

	// compute area size feature
	float f4 = f.areas[PLANE_SIDE_ON] / (f.areas[PLANE_SIDE_FRONT] + f.areas[PLANE_SIDE_BACK] + f.areas[PLANE_SIDE_ON]);
	//float w4 = (float)listsize / (float)numpolygons;
	float w4 = 1.0f;

	// compute balanced area feature
	float f5 = 1.0f - (fabs(f.areas[PLANE_SIDE_FRONT] - f.areas[PLANE_SIDE_BACK]) / (f.areas[PLANE_SIDE_FRONT] + f.areas[PLANE_SIDE_BACK]));
	// a plane that has no front and back sides is already balanced
	if (f.areas[PLANE_SIDE_FRONT] - f.areas[PLANE_SIDE_BACK] == 0.0f)
		f5 = 1.0f;
	float w5 = 1.0f;

	// final weight adjustment
	w1 = 1.0f * w1;
	w2 = 0.0f * w2;
	w3 = 0.5f * w3;
	w4 = 0.1f * w4;
	w5 = 0.4f * w5;

	// compute score
	//Message("score: %f %f %f %f %f\n", f1, f2, f3, f4, f5);
	float s = (w1 * f1) + (w2 * f2) + (w3 * f3) + (w4 * f4) + (w5 * f5);

	return s;
}

// fixme: get this to return a face as ChooseBestSplitFace
static plane_t ChooseBestSplitPlane(bsppoly_t *list, bool *areahint)
{
	float bestscore = 0.0f;
	plane_t bestplane;
	bsppoly_t *p;
	
	bestscore = -1.0f;
	*areahint = false;
	for (p = list; p; p = p->next)
	{
		plane_t plane = PolygonPlane(p);
		float score = ComputeSplitPlaneScore(plane, p->areahint, list);

		if(score > bestscore)
		{
			bestscore	= score;
			bestplane	= plane;
		}
	}

	if (bestscore == -1)
		Error("best score is -1!\n");

	return bestplane;
}

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
}

static bsppoly_t *MakePolygonList(mapface_t *mapfaces)
{
	bsppoly_t	*list = NULL;
	bsppoly_t	**t = &list;

	for (mapface_t *f = mapfaces; f; f = f->next)
	{
		// allocate a new bsppoly
		bsppoly_t *p	= MallocBSPPoly(f->polygon);
		p->plane	= f->plane;
		p->box		= f->box;
		p->areahint	= f->areahint;
		
		// link it into the list
		(*t) = p;
		t = &p->next;
	}

	return list;
}

static bsptree_t *MakeEmptyTree()
{
	bsptree_t	*tree;
	
	tree = MallocTree();
	tree->root = MallocBSPNode(tree, NULL);

	return tree;
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

static box3 ClipNodeBoxWithPlane(box3 box, plane_t plane)
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
	bool areahint;
	plane = ChooseBestSplitPlane(list, &areahint);

	// split the polygon list
	PartitionPolygonList(plane, list, sides);

	node->plane = plane;
	node->areahint = areahint;
	
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

	Message("Building bsp tree\n");

	list = MakePolygonList(mapdata->faces);
	
	tree = MakeEmptyTree();
	
	// setup the root node bounding box
	{
		box = CalculatePolygonListBoundingBox(list);
		box.Expand(8.0f);
		tree->root->box = box;
	}
	
	BuildTreeRecursive(tree, tree->root, list, 1);

	Message("%i nodes\n", tree->numnodes);
	Message("%i leaves\n", tree->numleafs);
	Message("%i tree depth\n", tree->depth);
	
	return tree;
}


