static void Quat_Copy(float *a, float *b)
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[3];
}

static void Quat_Negate(float *a, float *b)
{
	a[0] = -b[0];
	a[1] = -b[1];
	a[2] = -b[2];
	a[3] = -b[3];
}

static float Quat_Dot(float *a, float *b)
{
	return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
}

static void Quat_Normalize(float *q)
{
	float len, invlen;

	len = sqrtf((q[0] * q[0]) + (q[1] * q[1]) + (q[2] * q[2]) + (q[3] * q[3]));
	invlen = 1.0f / len;

	q[0] *= invlen;
	q[1] *= invlen;
	q[2] *= invlen;
	q[3] *= invlen;
}

#if 1
static void Quat_NLerp(float* result, float *from, float *to, float t)
{
	float cosom = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
	
	float scale0 = 1.0f - t;
	float scale1 = ( cosom > 0.0f ) ? t : -t;
	
	result[0] = scale0 * from[0] + scale1 * to[0];
	result[1] = scale0 * from[1] + scale1 * to[1];
	result[2] = scale0 * from[2] + scale1 * to[2];
	result[3] = scale0 * from[3] + scale1 * to[3];
	
	float s = 1.0f / sqrtf( result[0] * result[0] + result[1] * result[1] + result[2] * result[2] + result[3] * result[3] );
	
	result[0] *= s;
	result[1] *= s;
	result[2] *= s;
	result[3] *= s;
}
#endif

#if 0
static void Quat_NLerp(float* result, float *from, float *to, float t)
{
	float dot = Quat_Dot(from, to);
	if(dot < 0.0f)
	{
		Quat_Negate(to, to);
	}

	result[0] = ((1 - t) * from[0]) + (t * to[0]);
	result[1] = ((1 - t) * from[1]) + (t * to[1]);
	result[2] = ((1 - t) * from[2]) + (t * to[2]);
	result[3] = ((1 - t) * from[3]) + (t * to[3]);

	Quat_Normalize(result);
}
#endif


static void JointToMatrix(md5jointmat_t *m, md5joint_t *j)
{
	float xx, xy, xz, xw;
	float yy, yz, yw;
	float zz, zw;

	xx = j->q[0] * j->q[0];
	xy = j->q[0] * j->q[1];
	xz = j->q[0] * j->q[2];
	xw = j->q[0] * j->q[3];

	yy = j->q[1] * j->q[1];
	yz = j->q[1] * j->q[2];
	yw = j->q[1] * j->q[3];

	zz = j->q[2] * j->q[2];
	zw = j->q[2] * j->q[3];

	m->m[0][0] = 1.0f - 2.0f * (yy + zz);
	m->m[0][1] = 2.0f * (xy - zw);
	m->m[0][2] = 2.0f * (xz + yw);
	m->m[0][3] = j->p[0];

	m->m[1][0] = 2.0f * (xy + zw);
	m->m[1][1] = 1.0f - 2.0f * (xx + zz);
	m->m[1][2] = 2.0f * (yz - xw);
	m->m[1][3] = j->p[1];

	m->m[2][0] = 2.0f * (xz - yw);
	m->m[2][1] = 2.0f * (yz + xw);
	m->m[2][2] = 1.0f - 2.0f * (xx + yy);
	m->m[2][3] = j->p[2];
}
#endif

static void JointMatrixMul(md5jointmat_t *c, md5jointmat_t *a, md5jointmat_t *b)
{
	c->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[0][1] * b->m[1][0] + a->m[0][2] * b->m[2][0];
	c->m[0][1] = a->m[0][0] * b->m[0][1] + a->m[0][1] * b->m[1][1] + a->m[0][2] * b->m[2][1];
	c->m[0][2] = a->m[0][0] * b->m[0][2] + a->m[0][1] * b->m[1][2] + a->m[0][2] * b->m[2][2];
	c->m[0][3] = a->m[0][0] * b->m[0][3] + a->m[0][1] * b->m[1][3] + a->m[0][2] * b->m[2][3] + a->m[0][3];

	c->m[1][0] = a->m[1][0] * b->m[0][0] + a->m[1][1] * b->m[1][0] + a->m[1][2] * b->m[2][0];
	c->m[1][1] = a->m[1][0] * b->m[0][1] + a->m[1][1] * b->m[1][1] + a->m[1][2] * b->m[2][1];
	c->m[1][2] = a->m[1][0] * b->m[0][2] + a->m[1][1] * b->m[1][2] + a->m[1][2] * b->m[2][2];
	c->m[1][3] = a->m[1][0] * b->m[0][3] + a->m[1][1] * b->m[1][3] + a->m[1][2] * b->m[2][3] + a->m[1][3];

	c->m[2][0] = a->m[2][0] * b->m[0][0] + a->m[2][1] * b->m[1][0] + a->m[2][2] * b->m[2][0];
	c->m[2][1] = a->m[2][0] * b->m[0][1] + a->m[2][1] * b->m[1][1] + a->m[2][2] * b->m[2][1];
	c->m[2][2] = a->m[2][0] * b->m[0][2] + a->m[2][1] * b->m[1][2] + a->m[2][2] * b->m[2][2];
	c->m[2][3] = a->m[2][0] * b->m[0][3] + a->m[2][1] * b->m[1][3] + a->m[2][2] * b->m[2][3] + a->m[2][3];
}

static void JointVertexMul(float *c, md5jointmat_t *m, float *v)
{
	c[0] = m->m[0][0] * v[0] + m->m[0][1] * v[1] + m->m[0][2] * v[2] + m->m[0][3];
	c[1] = m->m[1][0] * v[0] + m->m[1][1] * v[1] + m->m[1][2] * v[2] + m->m[1][3];
	c[2] = m->m[2][0] * v[0] + m->m[2][1] * v[1] + m->m[2][2] * v[2] + m->m[2][3];
}

