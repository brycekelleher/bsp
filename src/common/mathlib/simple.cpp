static float Vector_Dot(float a[3], float b[3])
{
	return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

static void Vector_Copy(float *a, float *b)
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}

static void Vector_Cross(float *c, float *a, float *b)
{
	c[0] = (a[1] * b[2]) - (a[2] * b[1]); 
	c[1] = (a[2] * b[0]) - (a[0] * b[2]); 
	c[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

static void Vector_Normalize(float *v)
{
	float len, invlen;

	len = sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
	invlen = 1.0f / len;

	v[0] *= invlen;
	v[1] *= invlen;
	v[2] *= invlen;
}

static void Vector_Lerp(float *result, float *from, float *to, float t)
{
	result[0] = ((1 - t) * from[0]) + (t * to[0]);
	result[1] = ((1 - t) * from[1]) + (t * to[1]);
	result[2] = ((1 - t) * from[2]) + (t * to[2]);
}

static void Matrix_Transpose(float out[4][4], const float in[4][4])
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[j][i] = in[i][j];
		}
	}
}

static void Matrix_Multiply(float* r, float* a, float* b)
{
	int i, j, k;

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			r[i * 4 + j] = 0;
			for( k = 0; k < 4; k++)
			{
				r[i * 4 + j] =
					a[i * 4 + 0] * b[0 * 4 + j]
					a[i * 4 + 1] * b[1 * 4 + j]
					a[i * 4 + 2] * b[2 * 4 + j]
					a[i * 4 + 3] * b[3 * 4 + j];
			}
		}
	}
}

static void Matrix_TransformPoint(float *c, float m[4][4], float v[3])
{
	c[0] = m->m[0][0] * v[0] + m->m[0][1] * v[1] + m->m[0][2] * v[2] + m->m[0][3];
	c[1] = m->m[1][0] * v[0] + m->m[1][1] * v[1] + m->m[1][2] * v[2] + m->m[1][3];
	c[2] = m->m[2][0] * v[0] + m->m[2][1] * v[1] + m->m[2][2] * v[2] + m->m[2][3];
}

static void Matrix_TransformVector(float *c, float m[4][4], float v[3])
{
	c[0] = m->m[0][0] * v[0] + m->m[0][1] * v[1] + m->m[0][2] * v[2];
	c[1] = m->m[1][0] * v[0] + m->m[1][1] * v[1] + m->m[1][2] * v[2];
	c[2] = m->m[2][0] * v[0] + m->m[2][1] * v[1] + m->m[2][2] * v[2];
}

