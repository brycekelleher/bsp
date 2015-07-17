#include <math.h>
#include "vec2.h"

vec2::vec2()
{}

vec2::vec2(const float x, const float y)
	: x(x), y(y)
{}

vec2::operator float*()
{
	return &x;
}

// read from an indexed element
float vec2::operator[](int i) const
{
	return (&this->x)[i];
}

// write to an indexed element
float& vec2::operator[](int i)
{
	return (&this->x)[i];
}
// unary operators
vec2 vec2::operator-() const
{
	//Negate();
	return vec2(-x, -y);
}

// functions
void vec2::Set(const float _x, const float _y)
{
	x = _x;
	y = _y;
}

void vec2::MakeZero()
{
	x = 0; 
	y = 0;
}

bool vec2::IsZero()
{
	return (x == 0) && (y == 0);
}

bool vec2::IsNearlyZero()
{
	return((x < EPSILON_E4) && (y < EPSILON_E4));
}

void vec2::Normalize()
{
	float invl = 1.0f / sqrtf((x * x) + (y * y));
	
	x *= invl;
	y *= invl;
}

float vec2::Length()
{
	return sqrtf((x * x) + (y * y));
}

float vec2::LengthSquared()
{
	return (x * x) + (y * y);
}

float* vec2::Ptr()
{
	return &x;
}

const float* vec2::Ptr() const
{
	return &x;
}

// new operators
vec2 operator+(const vec2 a, const vec2 b)
{
	vec2 r;

	r.x = a.x + b.x;
	r.y = a.y + b.y;

	return r;
}

vec2 operator-(const vec2 a, const vec2 b)
{
	vec2 r;

	r.x = a.x - b.x;
	r.y = a.y - b.y;

	return r;
}

vec2 operator*(const vec2 a, const vec2 b)
{
	vec2 r;

	r.x = a.x * b.x;
	r.y = a.y * b.y;

	return r;
}

vec2 operator/(const vec2 a, const vec2 b)
{
	return vec2(a.x / b.x, a.y / b.y);
}

vec2 operator*(const float s, const vec2 v)
{
	vec2 r;

	r.x = s * v.x;
	r.y = s * v.y;

	return r;
}

vec2 operator*(const vec2 v, const float s)
{
	return operator*(s, v);
}

vec2 operator/(const vec2 v, const float s)
{
	float rs = 1.0f / s;
	return rs * v;
}

bool operator==(const vec2 a, const vec2 b)
{
	return (a.x == b.x) && (a.y == b.y);
}

bool operator!=(const vec2 a, const vec2 b)
{
	return (a.x != b.x) || (a.y != b.y);
}

float Length(vec2 v)
{
	return v.Length();
}

float LengthSquared(vec2 v)
{
	return v.LengthSquared();
}

float Dot(const vec2 a, const vec2 b)
{
	return (a.x * b.x) + (a.y * b.y);
}

vec2 Normalize(vec2 v)
{
	v. Normalize();
	return v;
}

vec2 Skew(vec2 a)
{
	return vec2(a.y, -a.x);
}

