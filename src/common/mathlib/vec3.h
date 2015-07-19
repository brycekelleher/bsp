#ifndef __VEC3_H__
#define __VEC3_H__

/*-----------------------------------------------------------------------------
	vec3
-----------------------------------------------------------------------------*/

class vec3
{
public:
	float	x;
	float	y;
	float	z;
			
	// constructors
	vec3();
	vec3(const float x, const float y, const float z);

	operator float*();
	float operator[](int i) const;
	float& operator[](int i);

	vec3 operator-() const;

	// functions
	void Set(const float _x, const float _y, const float _z);
	void MakeZero();
	bool IsZero();
	bool IsNearlyZero();
	void Normalize();
	float Length();
	float LengthSquared();
	float* Ptr();
	const float* Ptr() const;
	
	// static constants
	static vec3 zero;
	static vec3 one;
	static vec3 float_max;
	static vec3 float_min;
};

vec3 operator+(const vec3 a, const vec3 b);
vec3 operator-(const vec3 a, const vec3 b);
vec3 operator*(const vec3 a, const vec3 b);
vec3 operator/(const vec3 a, const vec3 b);
vec3 operator*(const float s, const vec3 v);
vec3 operator*(const vec3 v, const float s);
vec3 operator/(const vec3 v, const float s);
bool operator==(const vec3 a, const vec3 b);
bool operator!=(const vec3 a, const vec3 b);
float Length(vec3 v);
float LengthSquared(vec3 v);
float Dot(const vec3 a, const vec3 b);
vec3 Cross(const vec3& a, const vec3& b);
vec3 Normalize(vec3 v);
vec3 Abs(vec3 v);
float LargestComponet(vec3 v);
int LargestComponentIndex(vec3 v);

extern vec3 vec3_zero;
extern vec3 vec3_one;
extern vec3 vec3_float_max;
extern vec3 vec3_float_min;

#endif

