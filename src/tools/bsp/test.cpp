#include "bsp.h"

typedef void (*bspcallback_t)(bspnode_t *n, void *data);

static void IterateLeafs(bspnode_t *node, bspcallback_t func, void *data)
{
	if (!node->children[0] && !node->children[1])
	{
		func(node, data);
		return;
	}

	IterateLeafs(node->children[0], func, data);
	IterateLeafs(node->children[1], func, data);
}

void TreeIterateLeafs(bsptree_t *tree, bspcallback_t func, void *data)
{
	IterateLeafs(tree->root, func, data);
}

static void WalkWithBox(bspnode_t *n, box3 box, bspcallback_t callback, void *data)
{
	// if this is a leaf then callback
	if(!n->children[0] && !n->children[1])
	{
		callback(n, data);
		return;
	}
	
	int side = n->plane.BoxOnPlaneSide(box, CLIP_EPSILON);
	
	if(side == PLANE_SIDE_FRONT)
		WalkWithBox(n->children[0], box, callback, data);
	else if (side == PLANE_SIDE_BACK)
		WalkWithBox(n->children[1], box, callback, data);
	else if(side == PLANE_SIDE_CROSS)
	{
		WalkWithBox(n->children[0], box, callback, data);
		WalkWithBox(n->children[1], box, callback, data);
	}
}

void TreeWalkWithBox(bsptree_t *tree, box3 box, bspcallback_t callback, void *data)
{
	WalkWithBox(tree->root, box, callback, data);
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

