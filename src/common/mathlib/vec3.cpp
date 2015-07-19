#include <math.h>
#include <float.h>
#include "vec3.h"

#define		EPSILON_E4	(1e-4)
#define		EPSILON_E8	(1e-8)

vec3 vec3_zero		= vec3(0.0f, 0.0f, 0.0f);
vec3 vec3_one		= vec3(1.0f, 0.0f, 0.0f);
vec3 vec3_float_max	= vec3(FLT_MAX, FLT_MAX, FLT_MAX);
vec3 vec3_float_min	= vec3(FLT_MIN, FLT_MIN, FLT_MIN);

vec3 vec3::zero		= vec3(0.0f, 0.0f, 0.0f);
vec3 vec3::one		= vec3(1.0f, 0.0f, 0.0f);
vec3 vec3::float_max	= vec3(FLT_MAX, FLT_MAX, FLT_MAX);
vec3 vec3::float_min	= vec3(FLT_MIN, FLT_MIN, FLT_MIN);

vec3::vec3()
{}

vec3::vec3(const float x, const float y, const float z)
	: x(x), y(y), z(z)
{}

vec3::operator float*()
{
	return &x;
}

// read from an indexed element
float vec3::operator[](int i) const
{
	return (&this->x)[i];
}

// write to an indexed element
float& vec3::operator[](int i)
{
	return (&this->x)[i];
}
// unary operators
vec3 vec3::operator-() const
{
	return vec3(-x, -y, -z);
}

// functions
void vec3::Set(const float _x, const float _y, const float _z)
{
	x = _x;
	y = _y;
	z = _z;
}

void vec3::MakeZero()
{
	x = 0; 
	y = 0;
	z = 0;
}

bool vec3::IsZero()
{
	return (x == 0) && (y == 0) && (z == 0);
}

bool vec3::IsNearlyZero()
{
	return (x < EPSILON_E4) && (y < EPSILON_E4) && (z < EPSILON_E4);
}

void vec3::Normalize()
{
	float invl = 1.0f / sqrtf((x * x) + (y * y) + (z * z));
	
	x *= invl;
	y *= invl;
	z *= invl;
}

float vec3::Length()
{
	return sqrtf((x * x) + (y * y) + (z * z));
}

float vec3::LengthSquared()
{
	return (x * x) + (y * y) + (z * z);
}

float* vec3::Ptr()
{
	return &x;
}

const float* vec3::Ptr() const
{
	return &x;
}

// new operators
vec3 operator+(const vec3 a, const vec3 b)
{
	vec3 r;

	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;

	return r;
}

vec3 operator-(const vec3 a, const vec3 b)
{
	vec3 r;

	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;

	return r;
}

vec3 operator*(const vec3 a, const vec3 b)
{
	vec3 r;

	r.x = a.x * b.x;
	r.y = a.y * b.y;
	r.z = a.z * b.z;

	return r;
}

vec3 operator/(const vec3 a, const vec3 b)
{
	return vec3(a.x / b.x, a.y / b.y, a.z / b.z);
}

vec3 operator*(const float s, const vec3 v)
{
	vec3 r;

	r.x = s * v.x;
	r.y = s * v.y;
	r.z = s * v.z;

	return r;
}

vec3 operator*(const vec3 v, const float s)
{
	return operator*(s, v);
}

vec3 operator/(const vec3 v, const float s)
{
	float rs = 1.0f / s;
	return rs * v;
}

bool operator==(const vec3 a, const vec3 b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

bool operator!=(const vec3 a, const vec3 b)
{
	return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

float Length(vec3 v)
{
	return v.Length();
}

float LengthSquared(vec3 v)
{
	return v.LengthSquared();
}

float Dot(const vec3 a, const vec3 b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

vec3 Cross(const vec3& a, const vec3& b)
{
	return vec3((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
}

vec3 Abs(vec3 v)
{
	return vec3(fabs(v[0]), fabs(v[1]), fabs(v[2]));
}

float LargestComponet(vec3 v)
{
	if (v[0] > v[1] && v[0] > v[2])
		return v[0];
	else if (v[1] > v[2])
		return v[1];
	else
		return v[2];
}

int LargestComponentIndex(vec3 v)
{
	if (v[0] > v[1] && v[0] > v[2])
		return 0;
	else if (v[1] > v[2])
		return 1;
	else
		return 2;
}

vec3 Normalize(vec3 v)
{
	v.Normalize();
	return v;
}

