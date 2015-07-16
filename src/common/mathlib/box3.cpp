#include <float.h>
#include "vec3.h"


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
};

box3::box3()
{}

box3::~box3()
{}

void box3::Clear()
{
	min = vec3( FLT_MAX,  FLT_MAX,  FLT_MAX);
	max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

bool box3::InsideOut()
{
	return (min[0] <= max[0]) && (min[1] <= max[1]) && (min[2] <= max[2]);
}

void box3::AddPoint(vec3 p)
{
	for (int i = 0; i < 3; i++)
	{
		if (p[i] < min[i])
			min[i] = p[i];
		if (p[i] > max[i])
			max[i] = p[i];
	}
}

void box3::AddBox(box3 b)
{
	for (int i = 0; i < 3; i++)
	{
		if (b.min[i] < min[i])
			min[i] = b.min[i];
		if (b.min[i] > max[i])
			max[i] = b.min[i];
	}
}

void box3::Expand(float d)
{
	min[0]	+= d;
	min[1]	+= d;
	min[2]	+= d;
	
	max[0]	+= d;
	max[1]	+= d;
	max[2]	+= d;
}

bool box3::ContainsPoints(vec3 p)
{
	if ((p[0] >= min[0] && p[0] <= max[0]) &&
	    (p[1] >= min[1] && p[1] <= max[1]) &&
	    (p[2] >= min[2] && p[2] <= max[2]))
	{
		return true;
	}

	return false;
}

bool box3::IntersectsBox(box3 b)
{
	if ((b.min[0] < max[0] || b.max[0] > min[0]) &&
	    (b.min[1] < max[1] || b.max[1] > min[1]))
	{
		return true;
	}

	return false;
}

void box3::FromPoints(vec3 *points, int numpoints)
{
	for(int i = 0; i < numpoints; i++)
		AddPoint(points[i]);
}
