#include <float.h>
#include "vec3.h"
#include "plane.h"
#include "box3.h"

box3::box3()
{
	Clear();
}

box3::~box3()
{}

void box3::Clear()
{
	min =  vec3::float_max;
	max = -vec3::float_max;
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
			max[i] = b.max[i];
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

bool box3::ContainsPoint(vec3 p)
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
	    (b.min[1] < max[1] || b.max[1] > min[1]) &&
	    (b.min[2] < max[2] || b.max[2] > min[2]))
	{
		return true;
	}

	return false;
}

void box3::FromPoints(vec3 *points, int numpoints)
{
	for (int i = 0; i < numpoints; i++)
		AddPoint(points[i]);
}

vec3 box3::Center()
{
	return 0.5f * (min + max);
}

vec3 box3::Size()
{
	return max - min;
}

int box3::LargestSide()
{
	vec3 v = Size();
	
	return LargestComponentIndex(v);
}

plane_t box3::PlaneForSide(int side)
{
	float normals[6][3] =
	{
		{ -1,  0,  0 },
		{  1,  0,  0 },
		{  0, -1,  0 },
		{  0,  1,  0 },
		{  0,  0, -1 },
		{  0,  0,  1 }
	};
	
	plane_t plane;
	plane[0] = normals[side][0];
	plane[1] = normals[side][1];
	plane[2] = normals[side][2];
	
	if((side & 1) == 0)
		plane[3] = min[side >> 1];
	else
		plane[3] = max[side >> 1];
	
	return plane;
}
