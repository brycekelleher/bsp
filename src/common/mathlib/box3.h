#ifndef __BOX3_H__
#define __BOX3_H__

class vec3;
class plane_t;

enum
{
	BOX_SIDE_X,
	BOX_SIDE_Y,
	BOX_SIDE_Z
};

enum
{
	BOX_SIDE_NEG_X,
	BOX_SIDE_POS_X,
	BOX_SIDE_NEG_Y,
	BOX_SIDE_POS_Y,
	BOX_SIDE_NEG_Z,
	BOX_SIDE_POS_Z
};

/*-----------------------------------------------------------------------------
	box3
-----------------------------------------------------------------------------*/

class box3
{
public:
	vec3	min;
	vec3	max;
	
	box3();
	~box3();
	
	void Clear();
	bool InsideOut();
	void AddPoint(vec3 p);
	void AddBox(box3 b);
	void Expand(float d);
	bool ContainsPoints(vec3 p);
	bool IntersectsBox(box3 b);
	void FromPoints(vec3 *points, int numpoints);
	vec3 Center();
	vec3 Size();
	int LargestSide();
	plane_t PlaneForSide(int side);
};

#endif

