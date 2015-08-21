#ifndef __PLANE_H__
#define __PLANE_H__

class vec3;
class box3;

enum
{
	PLANE_SIDE_FRONT	= 0,
	PLANE_SIDE_BACK		= 1,
	PLANE_SIDE_ON		= 2,
	PLANE_SIDE_CROSS	= 3
};

class plane_t
{
public:
	float	a;
	float	b;
	float	c;
	float	d;

	plane_t();
	plane_t(float a, float b, float c, float d);
	plane_t(vec3 normal, float distance);

	// read from an indexed element
	float operator[](int i) const;
	float& operator[](int i);
	
	void Reverse();
	void PositionThroughPoint(vec3 p);
	vec3 GetNormal();
	float GetDistance();
	void Zero();
	void FromPoints(vec3 p0, vec3 p1, vec3 p2);
	void FromVecs(vec3 v0, vec3 v1, vec3 p);

	float Distance(vec3& p);
	int PointOnPlaneSide(vec3& p, float epsilon);
	int BoxOnPlaneSide(box3 box, float epsilon);
	bool PlaneLineIntersection(vec3 start, vec3 end, vec3 *hitpoint);
	bool RayIntersection(vec3 start, vec3 dir, float* fraction);
	bool IsAxial();
	
	plane_t operator-();
};

float Distance(plane_t plane, vec3 x);
int PointOnPlaneSide(plane_t plane, vec3& p, const float epsilon);
int BoxOnPlaneSide(plane_t plane, box3 box, float epsilon);

#endif
