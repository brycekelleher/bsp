#include "bsp.h"

// if the polygon sits on the plane then send the plane down the front or back side depending on whether
// the polygon normal faces the same direction as the plane
static void FilterPolygonIntoLeaf(bspnode_t *n, polygon_t *p)
{
	if (!n->children[0] && !n->children[1])
	{
		//Message("found empty node %#p\n", n);

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
		float dot = Dot(n->plane.GetNormal(), Polygon_Normal(p));

		// map 0 to the front child and 1 to the back child
		int facing = (dot > 0.0f ? 0 : 1);

		FilterPolygonIntoLeaf(n->children[facing], p);
	}
	else if (side == PLANE_SIDE_CROSS)
	{
		polygon_t *f, *b;
		Polygon_SplitWithPlane(p, n->plane, CLIP_EPSILON, &f, &b);

		FilterPolygonIntoLeaf(n->children[0], f);
		FilterPolygonIntoLeaf(n->children[1], b);
	}
}

void MarkEmptyLeafs(bsptree_t *tree)
{
	// iterate all map faces, filter them into the tree
	for (mapface_t *f = mapdata->faces; f; f = f->next)
	{
		// fixme: need a method to filter only structural polygons
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
	leaf->areanext = area->leafs;
	area->leafs = leaf;
	area->numleafs++;

	// link the area to this leaf
	leaf->area = area;

	//Message("adding leaf %#p\n", leaf);

	for (portal_t *portal = leaf->portals; portal; portal = portal->leafnext)
	{
		// don't flow across solid/empty boundaries, leafs already assigned an area or across areahints
		if (portal->srcleaf->empty ^ portal->dstleaf->empty)
			continue;
		if (portal->dstleaf->area)
			continue;
		if (portal->areahint)
			continue;

		// flow into the next leaf
		Walk(area, portal, portal->dstleaf);
	}
}

static void FindAreas(bsptree_t *tree)
{
	bspnode_t *leaf;

	// process all that aren't empty
	for (leaf = tree->leafs; leaf; leaf = leaf->leafnext)
	{
		if (leaf->area)
			continue;
		if (leaf->empty)
			continue;

		//Message("processing leaf %#p\n", leaf);

		// create a new area
		area_t *area = AllocArea(tree);
		Walk(area, NULL, leaf);

	}

	// process all empty leafs
	for (leaf = tree->leafs; leaf; leaf = leaf->leafnext)
	{
		if (leaf->area)
			continue;

		//Message("processing leaf %#p\n", leaf);

		// create a new area
		area_t *area = AllocArea(tree);
		tree->numemptyareas++;

		Walk(area, NULL, leaf);
	}
}

static void AddPortalsToAreas(bsptree_t *tree)
{
	for (portal_t *portal = tree->portals; portal; portal = portal->treenext)
	{
		// filter portals which aren't areahints and which aren't in empty areas and cross empty/solid boundaries
		if (!portal->areahint)
			continue;
		if (!portal->srcleaf->empty)
			continue;
		if (portal->srcleaf->empty ^ portal->dstleaf->empty)
			continue;

		area_t *a = portal->srcleaf->area;

		// link this portal into the area's list of portals
		portal->areanext = a->portals;
		a->portals = portal;
		a->numportals++;
	}
}

void BuildAreas(bsptree_t *tree)
{
	Message("Builing areas\n");

	FindAreas(tree);

	AddPortalsToAreas(tree);

	Message("%d areas\n", tree->numareas);
	Message("%d empty areas\n", tree->numemptyareas);
}

