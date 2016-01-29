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

#if 0
void LineQueryRecursive(bspnode_t *n, line_t *line)
{
	if (!n->children[0] && !n->children[1])
	{
		// are we going from empty to solid or solid to empty?
		if (lineq) // && (n->empty ^ lineq->empty))
		{
			printf("hit point at %f, %f\n", line->v[0][0], line->v[0][1]);
			WriteCross(fplineq, line->v[0]);
		}

		lineq = n;
		return;
	}

	int side = Line_OnPlaneSide(line, n->plane, globalepsilon);

	if (side == PLANE_SIDE_FRONT)
		LineQueryRecursive(n->children[0], line);
	else if (side == PLANE_SIDE_BACK)
		LineQueryRecursive(n->children[1], line);
	else if (side == PLANE_SIDE_CROSS)
	{
		line_t *f, *b;
		Line_SplitWithPlane(line, n->plane, globalepsilon, &f, &b);

		side = n->plane.PointOnPlaneSide(line->v[0], globalepsilon);
		if(side == PLANE_SIDE_FRONT)
		{
			LineQueryRecursive(n->children[0], f);
			LineQueryRecursive(n->children[1], b);
		}
		else
		{
			LineQueryRecursive(n->children[1], b);
			LineQueryRecursive(n->children[0], f);
		}
	}
	else if (side == PLANE_SIDE_ON)
	{
		// hmmm	
		LineQueryRecursive(n->children[0], line);
	}
}

static void LineQuery(bsptree_t *tree)
{
	fplineq = fopen("lineq.gld", "w");

	line_t *l = Line_Alloc();
	l->v[0][0] = 0.0f;
	l->v[0][1] = 0.0f;
	l->v[1][0] = 4096.0f;
	l->v[1][1] = 4096.0f;

	lineq = NULL;
	LineQueryRecursive(tree->root, l);

	fclose(fplineq);
}
#endif

#if 0
// takes a polygon as input an returns a list of face fragments and the leafs that they belong to
static leafface_t *PushFaceIntoTree(bspnode_t *n, polygon_t *p)
{
	bspnode_t	*nstack[256];
	polygon_t	*pstack[256];
	int		sp = 0;

	nstack[sp]	= n;
	pstack[sp]	= p;
	sp++;

	printf("processing face...\n");

	while (sp)
	{
		sp--;
		n = nstack[sp];
		p = pstack[sp];

		printf("n=%p, child0=%p, child1=%p\n", n, n->children[0], n->children[1]);

		if (!n->children[0] && !n->children[1])
		{
			// only emit polygons into empty leafs
			if (!n->empty)
				continue;

			printf("face in leaf %p\n", n);
			PrintPolygon(p);
			continue;
		}

		int side = Polygon_OnPlaneSide(p, n->plane, CLIP_EPSILON);

		if (side == PLANE_SIDE_FRONT)
		{
			nstack[sp]	= n->children[0];
			pstack[sp]	= p;
			sp++;
		}
		else if (side == PLANE_SIDE_BACK)
		{
			nstack[sp]	= n->children[1];
			pstack[sp]	= p;
			sp++;
		}
		else if (side == PLANE_SIDE_ON)
		{
			float dot = Dot(n->plane.GetNormal(), Polygon_Normal(p));

			// map 0 to the front child and 1 to the back child
			int facing = (dot > 0.0f ? 0 : 1);

			nstack[sp]	= n->children[facing];
			pstack[sp]	= p;
			sp++;
		}
		else if (side == PLANE_SIDE_CROSS)
		{
			polygon_t *f, *b;
			Polygon_SplitWithPlane(p, n->plane, CLIP_EPSILON, &f, &b);

			nstack[sp]	= n->children[0];
			pstack[sp]	= f;
			sp++;
			nstack[sp]	= n->children[1];
			pstack[sp]	= b;
			sp++;
		}
	}

	return NULL;
}
#endif


