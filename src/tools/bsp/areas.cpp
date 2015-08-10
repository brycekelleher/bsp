#include "bsp.h"

static vec3 Polygon_GetNormal(polygon_t *p)
{
	plane_t plane = Polygon_Plane(p);
	return plane.GetNormal();
}

// if the polygon sits on the plane then send the plane down the front or back side depending on whether
// the polygon normal faces the same direction as the plane
static void FilterPolygonIntoLeaf(bspnode_t *n, polygon_t *p)
{
	if (!n->children[0] && !n->children[1])
	{
		// this is a leaf node
		n->empty = true;
		return;
	}

	int side = Polygon_OnPlaneSide(p, n->plane, CLIP_EPSILON);

	if (side == PLANE_SIDE_FRONT)
		FilterPolygonIntoLeaf(n->children[0], p);
	else if (side == PLANE_SIDE_BACK)
		FilterPolygonIntoLeaf(n->children[1], p);
	else if (side == PLANE_SIDE_ON)
	{
		float dot = Dot(n->plane.GetNormal(), Polygon_GetNormal(p));

		// map 0 to the front child and 1 to the back child
		int facing = (dot > 0.0f ? 0 : 1);

		FilterPolygonIntoLeaf(n->children[facing], p);
	}
	else if (side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(p, n->plane, CLIP_EPSILON, &f, &b);

		FilterPolygonIntoLeaf(n, f);
		FilterPolygonIntoLeaf(n, b);
	}
}

void MarkEmptyLeafs(bsptree_t *tree)
{
	// Iterate all map faces, filter them into the tree
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// Fixme: need a method to filter only structural polygons
		if (f->areahint)
			continue;

		polygon_t *polygon = Polygon_Copy(f->polygon);

		FilterPolygonIntoLeaf(tree->root, polygon);
	}
}
