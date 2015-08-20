#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <math.h>
#include <GL/freeglut.h>
#include "vec3.h"
#include "box3.h"
#include "plane.h"

#define PI 3.14159265358979323846

// fixme: these are exported by toollib
FILE *FileOpenBinaryRead(const char *filename);
FILE *FileOpenBinaryWrite(const char *filename);
FILE *FileOpenTextRead(const char *filename);
FILE *FileOpenTextWrite(const char *filename);
bool FileExists(const char *filename);
int FileSize(FILE *fp);
void FileClose(FILE *fp);
void WriteFile(const char *filename, void *data, int numbytes);
int ReadFile(const char* filename, void **data);
void WriteBytes(void *data, int numbytes, FILE *fp);
int ReadBytes(void *buf, int numbytes, FILE *fp);

static float Max(float a, float b)
{
	return (a > b ? a : b);
}

static float Min(float a, float b)
{
	return (a < b ? a : b);
}

// ==============================================
// memory allocation

#define MEM_ALLOC_SIZE	32 * 1024 * 1024

typedef struct memstack_s
{
	unsigned char mem[MEM_ALLOC_SIZE];
	int allocated;

} memstack_t;

static memstack_t memstack;

void *Mem_Alloc(int numbytes)
{
	unsigned char *mem;
	
	if(memstack.allocated + numbytes > MEM_ALLOC_SIZE)
	{
		printf("Error: Mem: no free space available\n");
		abort();
	}

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	memset(mem, 0, numbytes);

	return mem;
}

void Mem_FreeStack()
{
	memstack.allocated = 0;
}

// ==============================================
// model loading

int ReadInt(FILE *fp)
{
	int i;

	ReadBytes(&i, sizeof(int), fp);
	return i;
}

float ReadFloat(FILE *fp)
{
	float f;

	ReadBytes(&f, sizeof(float), fp);
	return f;
}

typedef struct surfvertex_s
{
	float		xyz[3];
	float		color[4];
	float		normal[3];

} surfvertex_t;

// a triangle surface
typedef struct surf_s
{
	struct surf_s*	next;

	int		numvertices;
	surfvertex_t	*vertices;

	//int		numindicies;
	//surfvertex_t	*indicies;

} surf_t;

typedef struct bspnode_s
{
	plane_t		plane;
	box3		box;
	int		areanum;
	struct bspnode_s*	children[2];
	struct bspnode_s*	areanext;
	struct area_s*		area;

} bspnode_t;

typedef struct portal_s
{
	struct portal_s		*next;
	struct portal_s		*areanext;
	bspnode_t		*srcleaf;
	bspnode_t		*dstleaf;

	// fixme: should this use vec3?
	int 			numvertices;
	vec3			*vertices;

} portal_t;

typedef struct area_s
{
	int		numportals;
	portal_t	*portals;

	int		numleafs;
	bspnode_t	*leafs;

	int		numsurfaces;
	surf_t		*surfaces;

} area_t;

// model data
static int numareas;
static area_t		*areas;
static bspnode_t	*nodes;
static portal_t		*portals;

static void LoadNodes(FILE *fp)
{
	int numnodes = ReadInt(fp);
	int numleafs = ReadInt(fp);
	nodes = (bspnode_t*)Mem_Alloc(numnodes * sizeof(bspnode_t));

	for (int i = 0; i < numnodes; i++)
	{
		bspnode_t *n = nodes + i;

		// fixme: move into another function
		n->plane[0] = ReadFloat(fp);
		n->plane[1] = ReadFloat(fp);
		n->plane[2] = ReadFloat(fp);
		n->plane[3] = ReadFloat(fp);

		// fixme: move into another function
		n->box.min[0] = ReadFloat(fp);
		n->box.min[1] = ReadFloat(fp);
		n->box.min[2] = ReadFloat(fp);
		n->box.max[0] = ReadFloat(fp);
		n->box.max[1] = ReadFloat(fp);
		n->box.max[2] = ReadFloat(fp);

		// fixme: a -1 areanum gives a bad pointer
		int areanum = ReadInt(fp);
		n->area = areas + areanum;

		int childnum[2];
		childnum[0] = ReadInt(fp);
		childnum[1] = ReadInt(fp);
		n->children[0] = nodes + childnum[0];
		n->children[1] = nodes + childnum[1];
	}
}

static portal_t *LoadPortal(FILE *fp)
{
	portal_t *p = (portal_t*)Mem_Alloc(sizeof(portal_t));
	p->next = portals;
	portals = p;

	int srcleaf = ReadInt(fp);
	p->srcleaf = nodes + srcleaf;

	int dstleaf = ReadInt(fp);
	p->dstleaf = nodes + dstleaf;

	p->numvertices = ReadInt(fp);
	p->vertices = (vec3*)Mem_Alloc(p->numvertices * sizeof(vec3));
	for (int i = 0; i < p->numvertices; i++)
	{
		p->vertices[i][0] = ReadFloat(fp);
		p->vertices[i][1] = ReadFloat(fp);
		p->vertices[i][2] = ReadFloat(fp);
	}

	return p;
}

static vec3 Vec3FromFloat(float v[3])
{
	return vec3(v[0], v[1], v[2]);
}

static void CalculateNormals(surf_t *s)
{
	for (int i = 0; i < s->numvertices; i += 3)
	{
		vec3 area = vec3(0, 0, 0);
		for (int j = 0; j < 3; j++)
		{
			int k = (j + 1) % 3;
			
			vec3 vj = Vec3FromFloat(s->vertices[i + j].xyz);
			vec3 vk = Vec3FromFloat(s->vertices[i + k].xyz);
			
			area[2] += (vj[0] * vk[1]) - (vk[0] * vj[1]);
			area[0] += (vj[1] * vk[2]) - (vk[1] * vj[2]);
			area[1] += (vj[2] * vk[0]) - (vk[2] * vj[0]);
		}
	
		vec3 normal = Normalize(area);

		for (int j = 0; j < 3; j++)
		{
			s->vertices[i + j].normal[0] = normal[0];
			s->vertices[i + j].normal[1] = normal[1];
			s->vertices[i + j].normal[2] = normal[2];
		}
	}
}

static surf_t *LoadSurface(FILE *fp)
{
	surf_t *s = (surf_t*)Mem_Alloc(sizeof(surf_t));

	s->numvertices = ReadInt(fp);
	s->vertices = (surfvertex_t*)Mem_Alloc(s->numvertices * sizeof(surfvertex_t));

	for (int i = 0; i < s->numvertices; i++)
	{
		s->vertices[i].xyz[0] = ReadFloat(fp);
		s->vertices[i].xyz[1] = ReadFloat(fp);
		s->vertices[i].xyz[2] = ReadFloat(fp);

		s->vertices[i].color[0] = 1.0f;
		s->vertices[i].color[1] = 1.0f;
		s->vertices[i].color[2] = 1.0f;
		s->vertices[i].color[3] = 1.0f;
	}

	CalculateNormals(s);

	return s;
}

// fixme: this references a numareas global
static void LoadAreas(FILE *fp)
{
	numareas = ReadInt(fp);
	areas = (area_t*)Mem_Alloc(numareas * sizeof(area_t));

	for (int i = 0; i < numareas; i++)
	{
		area_t *a = areas + i;

		// read the leaf nodes
		a->numleafs = ReadInt(fp);
		for (int j = 0; j < a->numleafs; j++)
		{
			int leafnum = ReadInt(fp);
			bspnode_t *l = nodes + leafnum;
			l->area = a;
			
			// link into the area list
			l->areanext = a->leafs;
			a->leafs = l;
		}

		// read the portals
		a->numportals = ReadInt(fp);
		for (int j = 0; j < a->numportals; j++)
		{
			portal_t *p = LoadPortal(fp);

			// linkt the portal into the area list
			p->areanext = a->portals;
			a->portals = p;

			// fixme: link into the tree list?
		}

		// read the area surfaces
		a->numsurfaces = ReadInt(fp);
		for (int j = 0; j < a->numsurfaces; j++)
		{
			surf_t *s = LoadSurface(fp);

			// link the surface into the area list
			s->next = a->surfaces;
			a->surfaces = s;
		}
	}
}

static void LoadModel(FILE *fp)
{
	LoadNodes(fp);
	LoadAreas(fp);
}

//==============================================
// simulation code

static int framenum;

// Input
typedef struct inputstate_s
{
	int mousepos[2];
	int moused[2];
	bool lbuttondown;
	bool rbuttondown;
	bool keys[256];

} inputstate_t;

static inputstate_t input;

typedef struct viewstate_s
{
	float	angles[3];
	float	pos[3];

	float	znear;
	float	zfar;
	float	fov;

} viewstate_t;

static viewstate_t viewstate;

typedef struct tickcmd_s
{
	float	forwardmove;
	float	sidemove;
	float	upmove;
	float	anglemove[3];

} tickcmd_t;

static tickcmd_t gcmd;

static void SetupDefaultViewState()
{
	// look down negative z
	viewstate.angles[0]	= 0.0f;
	viewstate.angles[1]	= PI / 2.0f;
	viewstate.angles[2]	= 0.0f;
	
	viewstate.pos[0]	= 0.0f;
	viewstate.pos[1]	= 0.0f;
	viewstate.pos[2]	= 256.0f;

	viewstate.znear		= 3.0f;
	viewstate.zfar		= 4096.0f;
	viewstate.fov		= 90.0f;
}

static void VectorsFromSphericalAngles(float vectors[3][3], float angles[2])
{
	float cx, sx, cy, sy, cz, sz;

	cx = cosf(angles[0]);
	sx = sinf(angles[0]);
	cy = cosf(angles[1]);
	sy = sinf(angles[1]);
	cz = cosf(angles[2]);
	sz = sinf(angles[2]);

	vectors[0][0] = cy * cz;
	vectors[0][1] = sz;
	vectors[0][2] = -sy * cz;

	vectors[1][0] = (-cx * cy * sz) + (sx * sy);
	vectors[1][1] = cx * cz;
	vectors[1][2] = (cx * sy * sz) + (sx * cy);

	vectors[2][0] = (sx * cy * sz) + (cx * sy);
	vectors[2][1] = (-sx * cz);
	vectors[2][2] = (-sx * sy * sz) + (cx * cy);
}


// build a current command from the input state
// the 'input' structure contains the input events converted into state
static void BuildTickCmd()
{
	tickcmd_t *cmd = &gcmd;
	float scale;
	
	// move forward ~512 units each second (60 * 4.2)
	scale = 4.2f;

	cmd->forwardmove = 0.0f;
	cmd->sidemove = 0.0f;
	cmd->upmove = 0.0f;
	cmd->anglemove[0] = 0.0f;
	cmd->anglemove[1] = 0.0f;
	cmd->anglemove[2] = 0.0f;

	if (input.keys['w'])
		cmd->forwardmove += scale;
	if (input.keys['s'])
		cmd->forwardmove -= scale;
	if (input.keys['d'])
		cmd->sidemove += scale;
	if (input.keys['a'])
		cmd->sidemove -= scale;
	if (input.keys['f'])
		cmd->upmove += scale;
	if (input.keys['v'])
		cmd->upmove -= scale;

	// handle mouse movement
	if(input.lbuttondown)
	{
		cmd->anglemove[1] = -0.01f * (float)input.moused[0];
		cmd->anglemove[2] = -0.01f * (float)input.moused[1];
	}
}

// apply the tick command to the viewstate
static void DoMove()
{
	tickcmd_t *cmd = &gcmd;

	float vectors[3][3];
	VectorsFromSphericalAngles(vectors, viewstate.angles);

	viewstate.pos[0] += cmd->forwardmove * vectors[0][0];
	viewstate.pos[1] += cmd->forwardmove * vectors[0][1];
	viewstate.pos[2] += cmd->forwardmove * vectors[0][2];

	viewstate.pos[0] += cmd->sidemove * vectors[2][0];
	viewstate.pos[1] += cmd->sidemove * vectors[2][1];
	viewstate.pos[2] += cmd->sidemove * vectors[2][2];

	viewstate.pos[0] += cmd->upmove * vectors[2][0];
	viewstate.pos[1] += cmd->upmove * vectors[2][1];
	viewstate.pos[2] += cmd->upmove * vectors[2][2];

	viewstate.angles[0] += cmd->anglemove[0];
	viewstate.angles[1] += cmd->anglemove[1];
	viewstate.angles[2] += cmd->anglemove[2];

	if(viewstate.angles[2] >= PI / 2.0f)
		viewstate.angles[2] = (PI / 2.0f) - 0.001f;
	if(viewstate.angles[2] <= -PI/ 2.0f)
		viewstate.angles[2] = (-PI / 2.0f) + 0.001f;
}

static void SimulationTick()
{
	// build a command from the input events that occurred
	BuildTickCmd();
	
	// apply the inputs to the view state
	DoMove();
}

//==============================================
// OpenGL rendering code
//

// this stuff touches some of the simulation state (viewvectors, viewpos etc)
// guess it should really have an interface to extract that data?

// the global renderstate
typedef struct rs_s
{
	int	renderwidth;
	int	renderheight;

	float	fov;
	float	znear, zfar;

	// view position and view vectors
	float	pos[3];
	float	viewvectors[3][3];

	// derived values
	float	view[4][4];
	float	projection[4][4];
	float	clip[4][4];

} rs_t;

static rs_t	rs;

// draw buffer
typedef struct drawvertex_s
{
	float	xyz[3];
	float	normal[3];
	float	color[4];

} drawvertex_t;

typedef struct drawbuffer_s
{
	int numvertices;
	drawvertex_t *vertices;

} drawbuffer_t;

static drawbuffer_t drawbuffer;


static void CalculateNormalsAsColors(drawbuffer_t *b)
{
	for (int i = 0; i < b->numvertices; i++)
	{
		b->vertices[i].color[0] = 0.5f + 0.5f * b->vertices[i].normal[0];
		b->vertices[i].color[1] = 0.5f + 0.5f * b->vertices[i].normal[1];
		b->vertices[i].color[2] = 0.5f + 0.5f * b->vertices[i].normal[2];
		b->vertices[i].color[3] = 1.0f;
	}
}

static void CalculateLighting(drawbuffer_t *b)
{
	for (int i = 0; i < b->numvertices; i += 3)
	{
		vec3 v		= Vec3FromFloat(rs.viewvectors[0]);
		vec3 n		= Vec3FromFloat(b->vertices[i].normal);
		vec3 l		= -v;
		float ndotl	= Max(Dot(n, l), 0.0f);

		// scale and bias the color slightly
		ndotl		= 0.6 * ndotl + 0.4;

		// scale the color by the 'color'
		ndotl		= 0.75f * ndotl;

		for (int j = 0; j < 3; j++)
		{
			b->vertices[i + j].color[0] = ndotl;
			b->vertices[i + j].color[1] = ndotl;
			b->vertices[i + j].color[2] = ndotl;
			b->vertices[i + j].color[3] = 1.0f;
		}
	}
}

static void DrawBuffer(drawbuffer_t *b)
{
	glVertexPointer(3, GL_FLOAT, sizeof(drawvertex_t), b->vertices->xyz);
	glNormalPointer(GL_FLOAT, sizeof(drawvertex_t), b->vertices->normal);
	glColorPointer(4, GL_FLOAT, sizeof(drawvertex_t), b->vertices->color);

	glDrawArrays(GL_TRIANGLES, 0, b->numvertices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

// copy the vertex data in to the drawbuffer
static void CopySurface(drawbuffer_t *b, surf_t *s, int numvertices)
{
	if (!b->vertices)
		b->vertices = (drawvertex_t*)malloc(1024 * 1024 * sizeof(drawvertex_t));

	int base = b->numvertices;
	for (int i = 0; i < s->numvertices; i++)
	{
		b->vertices[base + i].xyz[0] = s->vertices[i].xyz[0];
		b->vertices[base + i].xyz[1] = s->vertices[i].xyz[1];
		b->vertices[base + i].xyz[2] = s->vertices[i].xyz[2];

		b->vertices[base + i].normal[0] = s->vertices[i].normal[0];
		b->vertices[base + i].normal[1] = s->vertices[i].normal[1];
		b->vertices[base + i].normal[2] = s->vertices[i].normal[2];

		b->vertices[base + i].color[0] = s->vertices[i].color[0];
		b->vertices[base + i].color[1] = s->vertices[i].color[1];
		b->vertices[base + i].color[2] = s->vertices[i].color[2];
		b->numvertices++;
	}
}

static void SetupDrawBuffer(drawbuffer_t *b)
{
	// flush the drawbuffer
	b->numvertices = 0;

	for (int i = 0; i < numareas; i++)
	{
		area_t *a = areas + i;

		if (!a->surfaces)
			continue;

		CopySurface(b, a->surfaces, a->surfaces->numvertices);
	}
}

static void DrawWireframe(drawbuffer_t *b)
{
	static float white[] = { 1, 1, 1 };
	static float black[] = { 0, 0, 0 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// draw solid color
	glColor3fv(white);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	DrawBuffer(b);

	// draw wireframe outline
	glColor3fv(black);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -2);
	DrawBuffer(b);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void DrawNormalsAsColors(drawbuffer_t *b)
{
	CalculateNormalsAsColors(b);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	DrawBuffer(b);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void DrawLit(drawbuffer_t *b)
{
	CalculateLighting(b);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	DrawBuffer(b);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}


static void MatrixMultiply(float out[4][4], const float a[4][4], const float b[4][4])
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[i][j] = 0.0f;
			for (int k = 0; k < 4; k++)
				out[i][j] += a[i][k] * b[k][j];
		}
	}
}
static void MatrixTranspose(float out[4][4], const float in[4][4])
{
	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			out[j][i] = in[i][j];
}

static void GL_LoadMatrix(float m[4][4])
{
	glLoadMatrixf((float*)m);
}

static void GL_LoadMatrixTranspose(float m[4][4])
{
	float t[4][4];

	MatrixTranspose(t, m);
	glLoadMatrixf((float*)t);
}

static void GL_MultMatrix(float m[4][4])
{
	glMultMatrixf((float*)m);
}

static void GL_MultMatrixTranspose(float m[4][4])
{
	float t[4][4];

	MatrixTranspose(t, m);
	glMultMatrixf((float*)t);
}

static void SetupViewVectors()
{
	VectorsFromSphericalAngles(rs.viewvectors, viewstate.angles);
}

static void SetupModelViewMatrix()
{
	// matrix to transform from look down x to looking down -z
	static float yrotate[4][4] =
	{
		{ 0, 0, 1, 0 },
		{ 0, 1, 0, 0 },
		{ -1, 0, 0, 0 },
		{ 0, 0, 0, 1 }
	};

	float matrix[4][4];
	matrix[0][0]	= rs.viewvectors[0][0];
	matrix[0][1]	= rs.viewvectors[0][1];
	matrix[0][2]	= rs.viewvectors[0][2];
	matrix[0][3]	= -(rs.viewvectors[0][0] * rs.pos[0]) - (rs.viewvectors[0][1] * rs.pos[1]) - (rs.viewvectors[0][2] * rs.pos[2]);

	matrix[1][0]	= rs.viewvectors[1][0];
	matrix[1][1]	= rs.viewvectors[1][1];
	matrix[1][2]	= rs.viewvectors[1][2];
	matrix[1][3]	= -(rs.viewvectors[1][0] * rs.pos[0]) - (rs.viewvectors[1][1] * rs.pos[1]) - (rs.viewvectors[1][2] * rs.pos[2]);

	matrix[2][0]	= rs.viewvectors[2][0];
	matrix[2][1]	= rs.viewvectors[2][1];
	matrix[2][2]	= rs.viewvectors[2][2];
	matrix[2][3]	= -(rs.viewvectors[2][0] * rs.pos[0]) - (rs.viewvectors[2][1] * rs.pos[1]) - (rs.viewvectors[2][2] * rs.pos[2]);

	matrix[3][0]	= 0.0f;
	matrix[3][1]	= 0.0f;
	matrix[3][2]	= 0.0f;
	matrix[3][3]	= 1.0f;

	MatrixMultiply(rs.view, yrotate, matrix);
}

static void SetupProjectionMatrix()
{
	float r, l, t, b;
	float fovx, fovy;

	fovx = rs.fov * (PI / 360.0f);
	float x = (rs.renderwidth / 2.0f) / atan(fovx);
	fovy = atan2(rs.renderheight / 2.0f, x);

	// Calcuate right, left, top and bottom values
	r = rs.znear * fovx;
	l = -r;

	t = rs.znear * fovy;
	b = -t;

	rs.projection[0][0] = (2.0f * rs.znear) / (r - l);
	rs.projection[0][1] = 0;
	rs.projection[0][2] = (r + l) / (r - l);
	rs.projection[0][3] = 0;

	rs.projection[1][0] = 0;
	rs.projection[1][1] = (2.0f * rs.znear) / (t - b);
	rs.projection[1][2] = (t + b) / (t - b);
	rs.projection[1][3] = 0;

	rs.projection[2][0] = 0;
	rs.projection[2][1] = 0;
	rs.projection[2][2] = -(rs.zfar + rs.znear) / (rs.zfar - rs.znear);
	rs.projection[2][3] = -2.0f * rs.zfar * rs.znear / (rs.zfar - rs.znear);

	rs.projection[3][0] = 0;
	rs.projection[3][1] = 0;
	rs.projection[3][2] = -1;
	rs.projection[3][3] = 0;
}

static void SetupGLMatrixState()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GL_LoadMatrixTranspose(rs.view);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GL_LoadMatrixTranspose(rs.projection);
}

static void SetupMatrices()
{
	rs.znear	= viewstate.znear;
	rs.zfar		= viewstate.zfar;
	rs.fov		= viewstate.fov;

	rs.pos[0]	= viewstate.pos[0];
	rs.pos[1]	= viewstate.pos[1];
	rs.pos[2]	= viewstate.pos[2];

	SetupViewVectors();

	SetupModelViewMatrix();

	SetupProjectionMatrix();

	SetupGLMatrixState();

	// build the clip matrix
	MatrixMultiply(rs.clip, rs.projection, rs.view);
}

#if 0
void MatrixMultiply(const float r[4][4], const float a[4][4], const float b[4][4])
{
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
}



// assumes that the bottom row of the matrix will produce a 1
void VertexTransform(const float r[3], const float m[4][4] const float v[3])
{
	r[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + m[0][3];
	r[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + m[1][3];
	r[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + m[2][3];
}



//portal code
void IntersectionOfBoxes()
{
	// for the minimum axis, take the max of the minumums
	// for the maxumum axis, take the min of the maximums

	// test if the box is actually valid
}

void ProjectPortal(portal_t *p)
{
	for (int i = 0; i < portal->numvertices; i++)
	{
		p->vertex[i]
}
#endif

static void DrawAxis()
{
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(32, 0, 0);

	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 32, 0);

	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 32);
	glEnd();
}

static void DrawVector(float *origin, float *dir)
{
	float s = 2.0f;

	glBegin(GL_LINES);
	{
		glVertex3f(origin[0], origin[1], origin[2]);
		glVertex3f(s * dir[0] + origin[0], s * dir[1] + origin[1], s * dir[2] + origin[2]);
	}
	glEnd();
}

static void DrawMatrix(float matrix[4][4])
{
	float vectors[4][3] =
	{
		{ matrix[0][0], matrix[1][0], matrix[2][0] },
		{ matrix[0][1], matrix[1][1], matrix[2][1] },
		{ matrix[0][2], matrix[1][2], matrix[2][2] },
		{ matrix[0][3], matrix[1][3], matrix[2][3] },
	};

	glColor3f(1, 0, 0);
	DrawVector(vectors[3], vectors[0]);
	glColor3f(0, 1, 0);
	DrawVector(vectors[3], vectors[1]);
	glColor3f(0, 0, 1);
	DrawVector(vectors[3], vectors[2]);
}

static void DrawPortal(portal_t *p)
{
	glColor3f(0, 1, 0);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < p->numvertices; i++)
		glVertex3fv(&p->vertices[i].x);
	glEnd();
}

static void DrawPortals()
{
	for (portal_t *p = portals; p; p = p->next)
		DrawPortal(p);
}

static void DrawWorld()
{
	SetupDrawBuffer(&drawbuffer);

	//DrawWireframe(&drawbuffer);
	//DrawNormalsAsColors(&drawbuffer);
	DrawLit(&drawbuffer);
}

static void Draw()
{
	DrawAxis();

	DrawWorld();

	DrawPortals();
}

static void BeginFrame()
{
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	SetupMatrices();
}

static void EndFrame()
{}

static void DrawFrame()
{
	BeginFrame();

	Draw();

	EndFrame();
}


//==============================================
// GLUT OS windowing code

static int mousepos[2];

static void ProcessInput()
{
	// mousepos has current "frame" mouse pos
	input.moused[0] = mousepos[0] - input.mousepos[0];
	input.moused[1] = mousepos[1] - input.mousepos[1];
	input.mousepos[0] = mousepos[0];
	input.mousepos[1] = mousepos[1];
}

static void DisplayFunc()
{
	DrawFrame();

	glutSwapBuffers();
}

static void KeyboardDownFunc(unsigned char key, int x, int y)
{
	input.keys[key] = true;
}

static void KeyboardUpFunc(unsigned char key, int x, int y)
{
	input.keys[key] = false;
}

static void ReshapeFunc(int w, int h)
{
	rs.renderwidth = w;
	rs.renderheight = h;

	glViewport(0, 0, rs.renderwidth, rs.renderheight);
}

static void MouseFunc(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON)
		input.lbuttondown = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		input.rbuttondown = (state == GLUT_DOWN);
}

static void MouseMoveFunc(int x, int y)
{
	mousepos[0] = x;
	mousepos[1] = y;
}

static void TimerFunc(int value)
{
	ProcessInput();

	// run the simulation code
	SimulationTick();

	// kick a redraw
	glutPostRedisplay();

	// schedule the next tick
	glutTimerFunc(16, TimerFunc, value);

	framenum++;
}

int GLUTMain(int argc, char *argv[])
{
	glutInit(&argc, argv);

	SetupDefaultViewState();
	
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("test");

	glutReshapeFunc(ReshapeFunc);
	glutDisplayFunc(DisplayFunc);
	glutKeyboardFunc(KeyboardDownFunc);
	glutKeyboardUpFunc(KeyboardUpFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MouseMoveFunc);
	glutPassiveMotionFunc(MouseMoveFunc);
	glutTimerFunc(16, TimerFunc, 0);

	glutMainLoop();
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("bspview bspfile\n");
		exit(EXIT_SUCCESS);
	}

	// load the model
	FILE *fp = FileOpenBinaryRead(argv[1]);
	LoadModel(fp);
	FileClose(fp);

	GLUTMain(argc, argv);

	return 0;
}

