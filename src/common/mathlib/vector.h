/*=============================================================================
	vector.h
============================================================================*/

#ifndef __VECTOR_H__
#define __VECTOR_H__

#define EPSILON_E4	(1e-4)
#define EPSILON_E8	(1e-8)

#include <math.h>


/*-----------------------------------------------------------------------------
	vec4
-----------------------------------------------------------------------------*/

class vec4
{
public:
	float	x;
	float	y;
	float	z;
	float	w;
			
	// constructors
	vec4();
	vec4(const float x, const float y, const float z, const float w);

	operator float*();
	float operator[](int i) const;
	float& operator[](int i);

	vec4 operator-() const;

	// functions
	void Set(const float _x, const float _y, const float _z, const float _w);
	void MakeZero();
	bool IsZero();
	bool IsNearlyZero();
	void Normalize();
	float Length();
	float LengthSquared();
	float* Ptr();
	const float* Ptr() const;
};

inline vec4::vec4()
{}

inline vec4::vec4(const float x, const float y, const float z, const float w)
	: x(x), y(y), z(z), w(w)
{}

inline vec4::operator float*()
{
	return &x;
}

// read from an indexed element
inline float vec4::operator[](int i) const
{
	return (&this->x)[i];
}

// write to an indexed element
inline float& vec4::operator[](int i)
{
	return (&this->x)[i];
}
// unary operators
inline vec4 vec4::operator-() const
{
	return vec4(-x, -y, -z, -w);
}

// functions
inline void vec4::Set(const float _x, const float _y, const float _z, const float _w)
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}

inline void vec4::MakeZero()
{
	x = 0; 
	y = 0;
	z = 0;
	w = 0;
}

inline bool vec4::IsZero()
{
	return (x == 0) && (y == 0) && (z == 0) && (w == 0);
}

inline bool vec4::IsNearlyZero()
{
	return (x < EPSILON_E4) && (y < EPSILON_E4) && (z < EPSILON_E4);
}

inline void vec4::Normalize()
{
	float invl = 1.0f / sqrtf((x * x) + (y * y) + (z * z) + (w * w));
	
	x *= invl;
	y *= invl;
	z *= invl;
	w *= invl;
}

inline float vec4::Length()
{
	return sqrtf((x * x) + (y * y) + (z * z) + (w * w));
}

inline float vec4::LengthSquared()
{
	return (x * x) + (y * y) + (z * z) + (w * w);
}

inline float* vec4::Ptr()
{
	return &x;
}

inline const float* vec4::Ptr() const
{
	return &x;
}

inline vec4 operator+(const vec4 a, const vec4 b)
{
	vec4 r;

	r.x = a.x + b.x;
	r.y = a.y + b.y;
	r.z = a.z + b.z;
	r.w = a.w + b.w;

	return r;
}

inline vec4 operator-(const vec4 a, const vec4 b)
{
	vec4 r;

	r.x = a.x - b.x;
	r.y = a.y - b.y;
	r.z = a.z - b.z;
	r.w = a.w - b.w;

	return r;
}

inline vec4 operator*(const vec4 a, const vec4 b)
{
	vec4 r;

	r.x = a.x * b.x;
	r.y = a.y * b.y;
	r.z = a.z * b.z;
	r.w = a.w * b.w;

	return r;
}

inline vec4 operator/(const vec4 a, const vec4 b)
{
	return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

inline vec4 operator*(const float s, const vec4 v)
{
	vec4 r;

	r.x = s * v.x;
	r.y = s * v.y;
	r.z = s * v.z;
	r.w = s * v.w;

	return r;
}

inline vec4 operator*(const vec4 v, const float s)
{
	return operator*(s, v);
}

inline vec4 operator/(const vec4 v, const float s)
{
	float rs = 1.0f / s;
	return rs * v;
}

inline bool operator==(const vec4 a, const vec4 b)
{
	return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

inline bool operator!=(const vec4 a, const vec4 b)
{
	return (a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w);
}

inline float Length(vec4 v)
{
	return v.Length();
}

inline float LengthSquared(vec4 v)
{
	return v.LengthSquared();
}

inline float Dot(const vec4 a, const vec4 b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline vec4 Normalize(vec4 v)
{
	v.Normalize();
	return v;
}

static vec4 vec4_zero(0.0f, 0.0f, 0.0f, 0.0f);

#endif

