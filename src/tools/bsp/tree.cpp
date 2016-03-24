#include "bsp.h"

// takes as input a list of polygons
// Choose a clipping plane (axial aligned, minimise splits)
// divide the list
// recurse on the list until no polygons remain

typedef struct bspface_s
{
	struct bspface_s	*next;
	polygon_t		*polygon;
	plane_t			plane;
	box3			box;
	bool			areahint;

} bspface_t;

// global list of bsp nodes
static bspnode_t	*bspnodes;

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

static int FaceOnPlaneSide(bspface_t *p, plane_t plane)
{
	return Polygon_OnPlaneSide(p->polygon, plane, CLIP_EPSILON);
}

static plane_t FacePlane(bspface_t *p)
{
	plane_t plane = Polygon_Plane(p->polygon);
	
	if (FaceOnPlaneSide(p, plane) != PLANE_SIDE_ON)
		Error("Polygon doesn't lie on calculated plane\n");
	
	return plane;
}

static bool CheckFaceOnPlane(bspface_t *p, plane_t plane)
{
	return (FaceOnPlaneSide(p, plane) == PLANE_SIDE_ON);
}

static box3 ClipBoxWithPlane(box3 box, plane_t plane)
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

static box3 BoundingBox(bspface_t *list)
{
	box3 box;
	
	for (; list; list = list->next)
		box.AddBox(list->box);
	
	return box;
}

static int Length(bspface_t *list)
{
	int i = 0;
	for(; list; list = list->next)
		i++;
	
	return i;
}

static plane_features_t ComputeSplitPlaneFeatures(plane_t plane, bool areahint, bspface_t *list)
{
	plane_features_t	f;

	f.areahint = areahint;
	f.axial = plane.IsAxial();
	f.sides[0] = f.sides[1] = f.sides[2] = f.sides[3] = 0;
	f.areas[0] = f.areas[1] = f.areas[2] = f.areas[3] = 0.0f;
	// zero_int_array(f.sides, 4)

	for(; list; list = list->next)
	{
		int side = FaceOnPlaneSide(list, plane);

		f.sides[side]	+= 1;
		f.areas[side]	+= Polygon_Area(list->polygon);
	}

	return f;
}

static float ComputeSplitPlaneScore(plane_t plane, bool areahint, bspface_t *list)
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
	// a plane that has no front and back sides has no balance
	if (f.areas[PLANE_SIDE_FRONT] - f.areas[PLANE_SIDE_BACK] == 0.0f)
		f5 = 0.0f;
	float w5 = 1.0f;

	// final weight adjustment
	w1 = 1.0f * w1;
	w2 = 0.0f * w2;
	w3 = 0.5f * w3;
	w4 = 0.1f * w4;
	w5 = 0.4f * w5;

	// compute score
	//Message("score: %f %f %f %f %f\n", f1, f2, f3, f4, f5);
	return (w1 * f1) + (w2 * f2) + (w3 * f3) + (w4 * f4) + (w5 * f5);
}

// fixme: get this to return a face as ChooseBestSplitFace
static plane_t ChooseBestSplitPlane(bspface_t *list, bool *areahint)
{
	float bestscore;
	plane_t bestplane;
	
	bestscore = -1.0f;
	*areahint = false;
	for (bspface_t *f = list; f; f = f->next)
	{
		plane_t plane = FacePlane(f);
		float score = ComputeSplitPlaneScore(plane, f->areahint, list);

		if (score > bestscore)
		{
			bestscore	= score;
			bestplane	= plane;
		}
	}

	if (bestscore == -1)
		Error("best score is -1!\n");

	return bestplane;
}

static bsptree_t *MallocTree()
{
	return (bsptree_t*)MallocZeroed(sizeof(bsptree_t));
}

static bspface_t *MallocBSPFace(polygon_t *polygon)
{
	bspface_t	*p;
	
	p = (bspface_t*)MallocZeroed(sizeof(bspface_t));
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

// BSPTree -> Integer
// consumes a bsp tree, walks the tree and returns the maximum depth
static int TreeMinDepth(bspnode_t *n)
{
#define MIN(a, b) (a < b ? a : b)

	if (!n->children[0] && !n->children[1])
		return 0;

	int fdepth = TreeMinDepth(n->children[0]);
	int bdepth = TreeMinDepth(n->children[1]);

	return MIN(fdepth + 1, bdepth + 1);
}

// BSPTree -> Integer
// consumes a bsp tree, walks the tree and returns the maximum depth
static int TreeMaxDepth(bspnode_t *n)
{
#define MAX(a, b) (a > b ? a : b)

	if (!n->children[0] && !n->children[1])
		return 0;

	int fdepth = TreeMaxDepth(n->children[0]);
	int bdepth = TreeMaxDepth(n->children[1]);

	return MAX(fdepth + 1, bdepth + 1);
}

// ==============================================
// Tree building code

static void SplitFace(bspface_t *p, plane_t plane, float epsilon, bspface_t **f, bspface_t **b)
{
	polygon_t *fp, *bp;
	
	*f = *b = NULL;
	
	// split the polygon
	Polygon_SplitWithPlane(p->polygon, plane, epsilon, &fp, &bp);
	
	if (fp)
	{
		*f = MallocBSPFace(fp);
		(*f)->plane	= p->plane;
		(*f)->box	= p->box;
		(*f)->areahint	= p->areahint;
	}
	if (bp)
	{
		*b = MallocBSPFace(bp);
		(*b)->plane	= p->plane;
		(*b)->box	= p->box;
		(*b)->areahint	= p->areahint;
	}
	
	// check that the split face sits in the original's plane
	if (*f && !CheckFaceOnPlane(*f, FacePlane(p)))
		Error("Front polygon doesn't sit on original plane after split\n");
	if (*b && !CheckFaceOnPlane(*b, FacePlane(p)))
		Error("Back polygon doesn't sit on original plane after split\n");
}

static bspface_t *MakeFaceList(mapface_t *mapfaces)
{
	bspface_t	*list = NULL;
	bspface_t	**t = &list;

	for (mapface_t *f = mapfaces; f; f = f->next)
	{
		// allocate a new bspface
		polygon_t *p		= Polygon_Copy(f->polygon);
		bspface_t *bspface	= MallocBSPFace(p);
		bspface->plane		= f->plane;
		bspface->box		= f->box;
		bspface->areahint	= f->areahint;
		
		// link it into the list
		(*t) = bspface;
		t = &bspface->next;
	}

	return list;
}

static bsptree_t *MakeEmptyTree(bspface_t *flist)
{
	bsptree_t	*tree;
	
	tree = MallocTree();
	tree->root = MallocBSPNode(tree, NULL);

	// setup the root node bounding box
	tree->root->box = BoundingBox(flist);
	tree->root->box.Expand(8.0f);

	return tree;
}

static void PartitionFaceList(plane_t plane, bspface_t *list, bspface_t **sides)
{
	sides[0] = sides[1] = NULL;
	
	for(bspface_t *f = list; f; f = f->next)
	{
		bspface_t *split[2];
		
		SplitFace(f, plane, CLIP_EPSILON, &split[0], &split[1]);
		
		// process the front (0) and back (1) splits
		for(int i = 0; i < 2; i++)
		{
			if(split[i])
			{
				split[i]->next = sides[i];
				sides[i] = split[i];
			}
		}
	}
}

// this will be called with a node and a list. The list will be of polygons fully
// contained within the node. Then the node is split with a plane
// Leaf nodes won't have a valid split plane
// Leaf nodes wont have valid child pointers
static void BuildTreeRecursive(bsptree_t *tree, bspnode_t *node, bspface_t *list)
{
	plane_t		plane;
	bspface_t	*sides[2];
	
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
	PartitionFaceList(plane, list, sides);

	node->plane = plane;
	node->areahint = areahint;
	
	// add two new nodes to the tree
	node->children[0] = MallocBSPNode(tree, node);
	node->children[1] = MallocBSPNode(tree, node);
	
	node->children[0]->box = ClipBoxWithPlane(node->box,  node->plane);
	node->children[1]->box = ClipBoxWithPlane(node->box, -node->plane);
	
	// recurse down the front and back sides
	BuildTreeRecursive(tree, node->children[0], sides[0]);
	BuildTreeRecursive(tree, node->children[1], sides[1]);
}

bsptree_t *BuildTree()
{
	bsptree_t	*tree;
	bspface_t	*flist;
	box3		box;

	Message("Building bsp tree\n");

	flist = MakeFaceList(mapdata->faces);
	
	tree = MakeEmptyTree(flist);
	
	BuildTreeRecursive(tree, tree->root, flist);
	tree->mindepth = TreeMinDepth(tree->root);
	tree->maxdepth = TreeMaxDepth(tree->root);

	Message("%i nodes\n", tree->numnodes);
	Message("%i leaves\n", tree->numleafs);
	Message("%i tree min depth\n", tree->mindepth);
	Message("%i tree max depth\n", tree->maxdepth);
	
	return tree;
}

