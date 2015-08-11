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
		Message("found empty node %#p\n", n);

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

// ==============================================
// Area building code

// pick the next leaf not assigned to an area
	// if not in an area
	// create an area
	// recurse through the portals
// don't fill across areahints
// dont' recurse from empty into solid

static area_t *AllocArea(bsptree_t *tree)
{
	area_t *a = (area_t*)MallocZeroed(sizeof(area_t));

	// link the area into the tree list
	a->next = tree->areas;
	tree->areas = a;

	tree->numareas++;

	return a;
}

static void Walk(area_t *area, portal_t *portal, bspnode_t *leaf)
{
	// link this leaf into the area's list of leaves
	area->leafs = leaf;
	leaf->areanext = area->leafs;

	leaf->area = area;

	if (leaf->empty)
		leaf->tree->numemptyareas++;

	Message("adding leaf %#p\n", leaf);

	for (portal_t *portal = leaf->portals; portal; portal = portal->leafnext)
	{
		// don't flow across solid/empty boundaries, leafs already assigned an area or across areahints
		if (leaf->empty ^ portal->dstleaf->empty)
			continue;
		if (portal->dstleaf->area)
			continue;
		if (portal->areahint)
			continue;

		Walk(area, portal, portal->dstleaf);
	}
}

void BuildAreas(bsptree_t *tree)
{
	Message("Builing areas\n");

	for (bspnode_t *leaf = tree->leafs; leaf; leaf = leaf->leafnext)
	{
		// don't process solid leafs (it would be nice to process empty before solid)
		if (leaf->area)
			continue;
		//if (!leaf->empty)
		//	continue;

		Message("processing leaf %#p\n", leaf);

		// create a new area
		area_t *area = AllocArea(tree);
		Walk(area, NULL, leaf);
	}

	Message("num areas: %d\n", tree->numareas);
	Message("num empty areas: %d\n", tree->numemptyareas);
}

