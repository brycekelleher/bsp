/*=============================================================================
	matrix.h
============================================================================*/

#ifndef __MATRIX_H__
#define __MATRIX_H__

#define MATRIX_INVERSE_EPSILON (0.01f)

class vec2;
class vec3;
class vec4;

/*-----------------------------------------------------------------------------
	mat2x2
-----------------------------------------------------------------------------*/

class mat2x2
{
public:

	vec2 m[2];

	// constructors
	mat2x2()
	{}
	mat2x2(vec2 x, vec2 y)
	{
		m[0] = x;
		m[1] = y;
	}
	mat2x2(float xx, float xy, float yx, float yy)
	{
		m[0].x = xx, m[0].y = xy;
		m[1].x = yx, m[1].y = yy;
	}
	mat2x2(float s)
	{
		m[0][0] = s, m[0][1] = s;
		m[1][0] = s, m[1][1] = s;
	}

	// functions
	inline void SetRow(int i, float x, float y)
	{
		m[i][0] = x;
		m[i][1] = y;
	}
	inline void SetRowFromVector(int i, float* v)
	{
		SetRow(i, v[0], v[1]);
	}
	inline void SetCol(int i, float x, float y)
	{
		m[0][i] = x;
		m[1][i] = y;
	}
	inline void SetColFromVector(int i, float *v)
	{
		SetCol(i, v[0], v[1]);
	}
	inline void Zero()
	{
		m[0].Set(0.0, 0.0);
		m[1].Set(0.0, 0.0);
	}
	inline void Negate()
	{
		m[0][0] = -m[0][0];
		m[0][1] = -m[0][1];

		m[1][0] = -m[1][0];
		m[1][1] = -m[1][1];
	}
	inline void Identity()
	{
		m[0][0] = 1.0f;
		m[0][1] = 0.0f;

		m[1][0] = 0.0f;
		m[1][1] = 1.0f;
	}
	inline void Transpose()
	{
		*this = mat2x2
		(
			m[0][0], m[1][0],
			m[0][1], m[1][1]
		);
	}

	inline void Invert()
	{
		float a = m[0][0], b = m[0][1], c = m[1][0], d = m[1][1];
		float det = a * d - b * c;

		if(det != 0.0f)
		{
			det = 1.0f / det;
		}

		mat2x2 B;
		B.m[0][0] =  det * d;	B[0][1] = -det * b;
		B.m[1][0] = -det * c;	B[1][1] =  det * a;
	
		*this = B;
	}	
	inline float* Ptr()
	{
		return &m[0].x;
	}
	inline const float* Ptr() const
	{
		return &m[0].x;
	}

	// negate all elements
	inline void operator-()
	{
		Negate();
	}
	
	// read from an indexed row
	inline vec2 operator[](int i) const
	{
		return m[i];
	}

	// write to an indexed row
	inline vec2& operator[](int i)
	{
		return m[i];
	}
};

inline mat2x2 operator+(const mat2x2 a, const mat2x2 b)
{
	mat2x2 r;

	r[0][0] = a[0][0] + b[0][0];
	r[0][1] = a[0][1] + b[0][1];

	r[1][0] = a[1][0] + b[1][0];
	r[1][1] = a[1][1] + b[1][1];

	return r;
}

inline mat2x2 operator-(const mat2x2 a, const mat2x2 b)
{
	mat2x2 r;

	r[0][0] = a[0][0] - b[0][0];
	r[0][1] = a[0][1] - b[0][1];

	r[1][0] = a[1][0] - b[1][0];
	r[1][1] = a[1][1] - b[1][1];

	return r;
}

inline mat2x2 operator*(const float f, const mat2x2 m)
{
	mat2x2 r;

	r[0][0] = f * m[0][0];
	r[0][1] = f * m[0][1];

	r[1][0] = f * m[1][0];
	r[1][1] = f * m[1][1];

	return r;
}

inline mat2x2 operator*(mat2x2 m, const float f)
{
	return operator*(f, m);
}

// matrix-matrix multiply
inline mat2x2 operator*(const mat2x2 a, const mat2x2 b)
{
	mat2x2 r;

	r[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0];
	r[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1];

	r[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0];
	r[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1];

	return r;
}

// matrix-vector post multiply
inline vec2 operator*(const mat2x2 m, const vec2 v)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[0][1];
	r[1] = v[0] * m[1][0] + v[1] * m[1][1];

	return r;
}

// vector-matrix pre multiply
inline vec2 operator*(const vec2 v, const mat2x2 m)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[1][0];
	r[1] = v[0] * m[0][1] + v[1] * m[1][1];

	return r;
}

inline bool operator==(const mat2x2& b, const mat2x2& a)
{
	return((b[0] == a[0]) && (b[1] == a[1]));
}

inline bool operator!=(const mat2x2& b, const mat2x2& a)
{
	return((b[0] != a[0]) || (b[1] != a[1]));
}

/*-----------------------------------------------------------------------------
	mat3x3
-----------------------------------------------------------------------------*/
#define MATRIX_INVERSE_EPSILON (0.01f)

class mat3x3
{
public:

	vec3 m[3];

	// constructors
	mat3x3()
	{}
	mat3x3(vec3 x, vec3 y, vec3 z)
	{
		m[0] = x;
		m[1] = y;
		m[2] = z;
	}
	mat3x3(float xx, float xy, float xz, float yx, float yy, float yz, float zx, float zy, float zz)
	{
		m[0].x = xx, m[0].y = xy, m[0].z = xz;
		m[1].x = yx, m[1].y = yy, m[1].z = yz;
		m[2].x = zx, m[2].y = zy, m[2].z = zz;
	}
	mat3x3(float s)
	{
		m[0][0] = s, m[0][1] = s, m[0][2] = s;
		m[1][0] = s, m[1][1] = s, m[1][2] = s;
		m[2][0] = s, m[2][1] = s, m[2][2] = s;
	}

	// functions
	inline void SetRow(int i, float x, float y, float z)
	{
		m[i][0] = x;
		m[i][1] = y;
		m[i][2] = z;
	}
	inline void SetRowFromVector(int i, float* v)
	{
		SetRow(i, v[0], v[1], v[2]);
	}
	inline void SetCol(int i, float x, float y, float z)
	{
		m[0][i] = x;
		m[1][i] = y;
		m[2][i] = z;
	}
	inline void SetColFromVector(int i, float *v)
	{
		SetCol(i, v[0], v[1], v[2]);
	}
	inline void Zero()
	{
		m[0].Set(0.0, 0.0, 0.0);
		m[1].Set(0.0, 0.0, 0.0);
		m[2].Set(0.0, 0.0, 0.0);
	}
	inline void Negate()
	{
		m[0] = -m[0];
		m[1] = -m[1];
		m[2] = -m[2];
	}
	inline void Identity()
	{
		m[0][0] = 1.0f;
		m[0][1] = 0.0f;
		m[0][2] = 0.0f;

		m[1][0] = 0.0f;
		m[1][1] = 1.0f;
		m[1][2] = 0.0f;

		m[2][0] = 0.0f;
		m[2][1] = 0.0f;
		m[2][2] = 1.0f;
	}
	inline void Transpose()
	{
		*this = mat3x3
		(
			m[0][0], m[1][0], m[2][0],
			m[0][1], m[1][1], m[2][1],
			m[0][2], m[1][2], m[2][2]
		);
	}
	inline float* Ptr()
	{
		return &m[0].x;
	}
	inline const float* Ptr() const
	{
		return &m[0].x;
	}

	// negate all elements
	inline void operator-()
	{
		Negate();
	}
	
	// read from an indexed row
	inline vec3 operator[](int i) const
	{
		return m[i];
	}

	// write to an indexed row
	inline vec3& operator[](int i)
	{
		return m[i];
	}
};

inline mat3x3 operator+(const mat3x3 a, const mat3x3 b)
{
	mat3x3 r;

	r[0][0] = a[0][0] + b[0][0];
	r[0][1] = a[0][1] + b[0][1];
	r[0][2] = a[0][2] + b[0][2];

	r[1][0] = a[1][0] + b[1][0];
	r[1][1] = a[1][1] + b[1][1];
	r[1][2] = a[1][2] + b[1][2];

	r[2][0] = a[2][0] + b[2][0];
	r[2][1] = a[2][1] + b[2][1];
	r[2][2] = a[2][2] + b[2][2];

	return r;
}

inline mat3x3 operator-(const mat3x3 a, const mat3x3 b)
{
	mat3x3 r;

	r[0][0] = a[0][0] - b[0][0];
	r[0][1] = a[0][1] - b[0][1];
	r[0][2] = a[0][2] - b[0][2];

	r[1][0] = a[1][0] - b[1][0];
	r[1][1] = a[1][1] - b[1][1];
	r[1][2] = a[1][2] - b[1][2];

	r[2][0] = a[2][0] - b[2][0];
	r[2][1] = a[2][1] - b[2][1];
	r[2][2] = a[2][2] - b[2][2];
	
	return r;
}

inline mat3x3 operator*(const float f, const mat3x3 m)
{
	mat3x3 r;

	r[0][0] = f * m[0][0];
	r[0][1] = f * m[0][1];
	r[0][2] = f * m[0][2];

	r[1][0] = f * m[1][0];
	r[1][1] = f * m[1][1];
	r[1][2] = f * m[1][2];

	r[2][0] = f * m[2][0];
	r[2][1] = f * m[2][1];
	r[2][2] = f * m[2][2];

	return r;
}

inline mat3x3 operator*(mat3x3 m, const float f)
{
	return operator*(f, m);
}

// matrix-matrix multiply
inline mat3x3 operator*(const mat3x3 a, const mat3x3 b)
{
	mat3x3 r;

	r[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0];
	r[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1];
	r[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2];

	r[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0];
	r[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1];
	r[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2];

	r[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0];
	r[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1];
	r[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2];

	return r;
}

// matrix-vector post multiply
inline vec2 operator*(const mat3x3 m, const vec2 v)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2];
	r[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2];
	r[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2];

	return r;
}

// vector-matrix pre multiply
inline vec2 operator*(const vec2 v, const mat3x3 m)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0];
	r[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1];
	r[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2];

	return r;
}

inline bool operator==(const mat3x3& b, const mat3x3& a)
{
	return (b[0] == a[0]) && (b[1] == a[1]);
}

inline bool operator!=(const mat3x3& b, const mat3x3& a)
{
	return (b[0] != a[0]) || (b[1] != a[1]);
}

/*-----------------------------------------------------------------------------
	mat4x4
-----------------------------------------------------------------------------*/
#define MATRIX_INVERSE_EPSILON (0.01f)

class mat4x4
{
public:

	vec4 m[3];

	// constructors
	mat4x4()
	{}
	mat4x4(vec4 x, vec4 y, vec4 z)
	{
		m[0] = x;
		m[1] = y;
		m[2] = z;
	}
	mat4x4(float xx, float xy, float xz, float xw, float yx, float yy, float yz, float yw, float zx, float zy, float zz, float zw, float wx, float wy, float wz, float ww)
	{
		m[0].x = xx, m[0].y = xy, m[0].z = xz, m[0].w = xw;
		m[1].x = yx, m[1].y = yy, m[1].z = yz, m[1].w = yw;
		m[2].x = zx, m[2].y = zy, m[2].z = zz, m[2].w = zw;
		m[3].x = wx, m[2].y = wy, m[3].z = wz, m[3].w = ww;
	}
	mat4x4(float s)
	{
		m[0][0] = s, m[0][1] = s, m[0][2] = s, m[0][3] = s;
		m[1][0] = s, m[1][1] = s, m[1][2] = s, m[1][3] = s;
		m[2][0] = s, m[2][1] = s, m[2][2] = s, m[2][3] = s;
		m[3][0] = s, m[3][1] = s, m[3][2] = s, m[3][3] = s;
	}

	// functions
	inline void SetRow(int i, float x, float y, float z, float w)
	{
		m[i][0] = x;
		m[i][1] = y;
		m[i][2] = z;
		m[i][3] = w;
	}
	inline void SetRowFromVector(int i, float* v)
	{
		SetRow(i, v[0], v[1], v[2], v[3]);
	}
	inline void SetCol(int i, float x, float y, float z, float w)
	{
		m[0][i] = x;
		m[1][i] = y;
		m[2][i] = z;
		m[3][i] = w;
	}
	inline void SetColFromVector(int i, float *v)
	{
		SetCol(i, v[0], v[1], v[2], v[3]);
	}
	inline void Zero()
	{
		m[0].Set(0.0, 0.0, 0.0, 0.0);
		m[1].Set(0.0, 0.0, 0.0, 0.0);
		m[2].Set(0.0, 0.0, 0.0, 0.0);
		m[3].Set(0.0, 0.0, 0.0, 0.0);
	}
	inline void Negate()
	{
		m[0] = -m[0];
		m[1] = -m[1];
		m[2] = -m[2];
		m[3] = -m[3];
	}
	inline void Identity()
	{
		m[0][0] = 1.0f;
		m[0][1] = 0.0f;
		m[0][2] = 0.0f;
		m[0][3] = 0.0f;

		m[1][0] = 0.0f;
		m[1][1] = 1.0f;
		m[1][2] = 0.0f;
		m[1][3] = 0.0f;

		m[2][0] = 0.0f;
		m[2][1] = 0.0f;
		m[2][2] = 1.0f;
		m[2][3] = 0.0f;

		m[3][0] = 0.0f;
		m[3][1] = 0.0f;
		m[3][2] = 0.0f;
		m[3][3] = 1.0f;
	}
	inline void Transpose()
	{
		*this = mat4x4
		(
			m[0][0], m[1][0], m[2][0], m[3][0],
			m[0][1], m[1][1], m[2][1], m[3][1],
			m[0][2], m[1][2], m[2][2], m[3][2],
			m[0][3], m[1][3], m[2][3], m[3][3]
		);
	}
	inline float* Ptr()
	{
		return &m[0].x;
	}
	inline const float* Ptr() const
	{
		return &m[0].x;
	}

	// negate all elements
	inline void operator-()
	{
		Negate();
	}
	
	// read from an indexed row
	inline vec4 operator[](int i) const
	{
		return m[i];
	}

	// write to an indexed row
	inline vec4& operator[](int i)
	{
		return m[i];
	}

	bool Invert()
	{
		// 84+4+16 = 104 multiplications
		//			   1 division
		double det, invDet;

		// 2x2 sub-determinants required to calculate 4x4 determinant
		float det2_01_01 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
		float det2_01_02 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
		float det2_01_03 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
		float det2_01_12 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
		float det2_01_13 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
		float det2_01_23 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

		// 3x3 sub-determinants required to calculate 4x4 determinant
		float det3_201_012 = m[2][0] * det2_01_12 - m[2][1] * det2_01_02 + m[2][2] * det2_01_01;
		float det3_201_013 = m[2][0] * det2_01_13 - m[2][1] * det2_01_03 + m[2][3] * det2_01_01;
		float det3_201_023 = m[2][0] * det2_01_23 - m[2][2] * det2_01_03 + m[2][3] * det2_01_02;
		float det3_201_123 = m[2][1] * det2_01_23 - m[2][2] * det2_01_13 + m[2][3] * det2_01_12;

		det = (- det3_201_123 * m[3][0] + det3_201_023 * m[3][1] - det3_201_013 * m[3][2] + det3_201_012 * m[3][3]);

		if (fabs(det) < MATRIX_INVERSE_EPSILON)
		{
			return false;
		}

		invDet = 1.0f / det;

		// remaining 2x2 sub-determinants
		float det2_03_01 = m[0][0] * m[3][1] - m[0][1] * m[3][0];
		float det2_03_02 = m[0][0] * m[3][2] - m[0][2] * m[3][0];
		float det2_03_03 = m[0][0] * m[3][3] - m[0][3] * m[3][0];
		float det2_03_12 = m[0][1] * m[3][2] - m[0][2] * m[3][1];
		float det2_03_13 = m[0][1] * m[3][3] - m[0][3] * m[3][1];
		float det2_03_23 = m[0][2] * m[3][3] - m[0][3] * m[3][2];

		float det2_13_01 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
		float det2_13_02 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
		float det2_13_03 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
		float det2_13_12 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
		float det2_13_13 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
		float det2_13_23 = m[1][2] * m[3][3] - m[1][3] * m[3][2];

		// remaining 3x3 sub-determinants
		float det3_203_012 = m[2][0] * det2_03_12 - m[2][1] * det2_03_02 + m[2][2] * det2_03_01;
		float det3_203_013 = m[2][0] * det2_03_13 - m[2][1] * det2_03_03 + m[2][3] * det2_03_01;
		float det3_203_023 = m[2][0] * det2_03_23 - m[2][2] * det2_03_03 + m[2][3] * det2_03_02;
		float det3_203_123 = m[2][1] * det2_03_23 - m[2][2] * det2_03_13 + m[2][3] * det2_03_12;

		float det3_213_012 = m[2][0] * det2_13_12 - m[2][1] * det2_13_02 + m[2][2] * det2_13_01;
		float det3_213_013 = m[2][0] * det2_13_13 - m[2][1] * det2_13_03 + m[2][3] * det2_13_01;
		float det3_213_023 = m[2][0] * det2_13_23 - m[2][2] * det2_13_03 + m[2][3] * det2_13_02;
		float det3_213_123 = m[2][1] * det2_13_23 - m[2][2] * det2_13_13 + m[2][3] * det2_13_12;

		float det3_301_012 = m[3][0] * det2_01_12 - m[3][1] * det2_01_02 + m[3][2] * det2_01_01;
		float det3_301_013 = m[3][0] * det2_01_13 - m[3][1] * det2_01_03 + m[3][3] * det2_01_01;
		float det3_301_023 = m[3][0] * det2_01_23 - m[3][2] * det2_01_03 + m[3][3] * det2_01_02;
		float det3_301_123 = m[3][1] * det2_01_23 - m[3][2] * det2_01_13 + m[3][3] * det2_01_12;

		m[0][0] = - det3_213_123 * invDet;
		m[1][0] = + det3_213_023 * invDet;
		m[2][0] = - det3_213_013 * invDet;
		m[3][0] = + det3_213_012 * invDet;

		m[0][1] = + det3_203_123 * invDet;
		m[1][1] = - det3_203_023 * invDet;
		m[2][1] = + det3_203_013 * invDet;
		m[3][1] = - det3_203_012 * invDet;

		m[0][2] = + det3_301_123 * invDet;
		m[1][2] = - det3_301_023 * invDet;
		m[2][2] = + det3_301_013 * invDet;
		m[3][2] = - det3_301_012 * invDet;

		m[0][3] = - det3_201_123 * invDet;
		m[1][3] = + det3_201_023 * invDet;
		m[2][3] = - det3_201_013 * invDet;
		m[3][3] = + det3_201_012 * invDet;

		return true;
	}
};

inline mat4x4 operator+(const mat4x4 a, const mat4x4 b)
{
	mat4x4 r;

	r[0][0] = a[0][0] + b[0][0];
	r[0][1] = a[0][1] + b[0][1];
	r[0][2] = a[0][2] + b[0][2];
	r[0][3] = a[0][3] + b[0][3];

	r[1][0] = a[1][0] + b[1][0];
	r[1][1] = a[1][1] + b[1][1];
	r[1][2] = a[1][2] + b[1][2];
	r[1][3] = a[1][3] + b[1][3];

	r[2][0] = a[2][0] + b[2][0];
	r[2][1] = a[2][1] + b[2][1];
	r[2][2] = a[2][2] + b[2][2];
	r[2][3] = a[2][3] + b[2][3];

	r[3][0] = a[3][0] + b[3][0];
	r[3][1] = a[3][1] + b[3][1];
	r[3][2] = a[3][2] + b[3][2];
	r[3][3] = a[3][3] + b[3][3];

	return r;
}

inline mat4x4 operator-(const mat4x4 a, const mat4x4 b)
{
	mat4x4 r;

	r[0][0] = a[0][0] - b[0][0];
	r[0][1] = a[0][1] - b[0][1];
	r[0][2] = a[0][2] - b[0][2];
	r[0][3] = a[0][3] - b[0][3];

	r[1][0] = a[1][0] - b[1][0];
	r[1][1] = a[1][1] - b[1][1];
	r[1][2] = a[1][2] - b[1][2];
	r[1][3] = a[1][3] - b[1][3];

	r[2][0] = a[2][0] - b[2][0];
	r[2][1] = a[2][1] - b[2][1];
	r[2][2] = a[2][2] - b[2][2];
	r[2][3] = a[2][3] - b[2][3];
	
	r[3][0] = a[3][0] - b[3][0];
	r[3][1] = a[3][1] - b[3][1];
	r[3][2] = a[3][2] - b[3][2];
	r[3][3] = a[3][3] - b[3][3];

	return r;
}

inline mat4x4 operator*(const float f, const mat4x4 m)
{
	mat4x4 r;

	r[0][0] = f * m[0][0];
	r[0][1] = f * m[0][1];
	r[0][2] = f * m[0][2];
	r[0][3] = f * m[0][3];

	r[1][0] = f * m[1][0];
	r[1][1] = f * m[1][1];
	r[1][2] = f * m[1][2];
	r[1][3] = f * m[1][3];

	r[2][0] = f * m[2][0];
	r[2][1] = f * m[2][1];
	r[2][2] = f * m[2][2];
	r[2][3] = f * m[2][3];

	r[3][0] = f * m[3][0];
	r[3][1] = f * m[3][1];
	r[3][2] = f * m[3][2];
	r[3][3] = f * m[3][3];

	return r;
}

inline mat4x4 operator*(mat4x4 m, const float f)
{
	return operator*(f, m);
}

// matrix-matrix multiply
inline mat4x4 operator*(const mat4x4 a, const mat4x4 b)
{
	mat4x4 r;

	r[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	r[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	r[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	r[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	r[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	r[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	r[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	r[1][2] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	r[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	r[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	r[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	r[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

	r[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
	r[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
	r[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
	r[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];

	return r;
}

// matrix-vector post multiply
inline vec2 operator*(const mat4x4 m, const vec2 v)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + v[3] * m[0][3];
	r[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + v[3] * m[1][3];
	r[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + v[3] * m[2][3];
	r[3] = v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m[3][2] + v[3] * m[3][3];

	return r;
}

// vector-matrix pre multiply
inline vec2 operator*(const vec2 v, const mat4x4 m)
{
	vec2 r;

	r[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
	r[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
	r[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
	r[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];

	return r;
}

inline bool operator==(const mat4x4& b, const mat4x4& a)
{
	return (b[0] == a[0]) && (b[1] == a[1]);
}

inline bool operator!=(const mat4x4& b, const mat4x4& a)
{
	return (b[0] != a[0]) || (b[1] != a[1]);
}

#endif

