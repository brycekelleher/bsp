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
void Error(const char *error, ...);
void Warning(const char *warning, ...);

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
		Error("Error: Mem: no free space available\n");

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	memset(mem, 0, numbytes);

	return mem;
}

void Mem_FreeStack()
{
	memstack.allocated = 0;
}

// ________________________________________________________________________________ 
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

	int		numindicies;
	int		*indicies;

} surf_t;

typedef struct bspnode_s
{
	plane_t		plane;
	box3		box;
	//int		areanum;
	bool		empty;

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

	int 			numvertices;
	vec3			*vertices;

} portal_t;

typedef struct area_s
{
	struct area_s	*next;
	struct area_s	*visiblenext;

	int		numportals;
	portal_t	*portals;

	int		numleafs;
	bspnode_t	*leafs;

	int		numsurfaces;
	surf_t		*surfaces;

	// used to test if we're flowing back into an area we've already been to
	int		areavisited;

} area_t;

// model data
static int numareas;
static area_t		*areas;
static bspnode_t	*nodes;
static portal_t		*portals;
static area_t		*arealist;
static area_t		*visibleareas;
static surf_t		*surfaces;

static void LoadNodes(FILE *fp)
{
	int numnodes = ReadInt(fp);
	int numleafs = ReadInt(fp);
	nodes = (bspnode_t*)Mem_Alloc(numnodes * sizeof(bspnode_t));

	for (int i = 0; i < numnodes; i++)
	{
		bspnode_t *n = nodes + i;

		int childnum[2];
		childnum[0] = ReadInt(fp);
		childnum[1] = ReadInt(fp);

		n->plane[0] = ReadFloat(fp);
		n->plane[1] = ReadFloat(fp);
		n->plane[2] = ReadFloat(fp);
		n->plane[3] = ReadFloat(fp);

		n->box.min[0] = ReadFloat(fp);
		n->box.min[1] = ReadFloat(fp);
		n->box.min[2] = ReadFloat(fp);
		n->box.max[0] = ReadFloat(fp);
		n->box.max[1] = ReadFloat(fp);
		n->box.max[2] = ReadFloat(fp);

		n->empty = false;

		// setup the child pointers
		n->children[0] = nodes + childnum[0];
		n->children[1] = nodes + childnum[1];
		if (childnum[0] == -1)
			n->children[0] = NULL;
		if (childnum[1] == -1)
			n->children[1] = NULL;
	}
}

static void LoadPortals(FILE *fp)
{
	int numportals = ReadInt(fp);
	for (int i = 0; i < numportals; i++)
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
	}
}

static void LoadAreas(FILE *fp)
{
	numareas = ReadInt(fp);
	areas = (area_t*)Mem_Alloc(numareas * sizeof(area_t));

	for (int i = 0; i < numareas; i++)
	{
		area_t *a = areas + i;

		// link the area into the global list
		a->next = arealist;
		arealist = a;

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
	}
}

static vec3 Vec3FromFloat(float v[3])
{
	return vec3(v[0], v[1], v[2]);
}

static void CalculateNormals(surf_t *s)
{
	for (int i = 0; i < s->numindicies; i += 3)
	{
		vec3 area = vec3(0, 0, 0);

		// for each edge of the triangle
		for (int j = 0; j < 3; j++)
		{
			int i0 = (j + 0) % 3 + i;
			int i1 = (j + 1) % 3 + i;
			
			vec3 v0 = Vec3FromFloat(s->vertices[s->indicies[i0]].xyz);
			vec3 v1 = Vec3FromFloat(s->vertices[s->indicies[i1]].xyz);
			
			area[2] += (v0[0] * v1[1]) - (v1[0] * v0[1]);
			area[0] += (v0[1] * v1[2]) - (v1[1] * v0[2]);
			area[1] += (v0[2] * v1[0]) - (v1[2] * v0[0]);
		}
	
		vec3 normal = Normalize(area);

		for (int j = 0; j < 3; j++)
		{
			s->vertices[s->indicies[i + j]].normal[0] = normal[0];
			s->vertices[s->indicies[i + j]].normal[1] = normal[1];
			s->vertices[s->indicies[i + j]].normal[2] = normal[2];
		}
	}
}

static void LoadRenderModel(FILE *fp)
{
	surf_t *s = (surf_t*)Mem_Alloc(sizeof(surf_t));
	s->next = surfaces;
	surfaces = s;

	// decode the name
	{
		char name[64];
		int len = ReadInt(fp);
		ReadBytes(name, len, fp);
	}

	// read the vertex block
	s->numvertices = ReadInt(fp);
	s->vertices = (surfvertex_t*)Mem_Alloc(s->numvertices * sizeof(surfvertex_t));

	for (int i = 0; i < s->numvertices; i++)
	{
		s->vertices[i].xyz[0] = ReadFloat(fp);
		s->vertices[i].xyz[1] = ReadFloat(fp);
		s->vertices[i].xyz[2] = ReadFloat(fp);
		s->vertices[i].normal[0] = ReadFloat(fp);
		s->vertices[i].normal[1] = ReadFloat(fp);
		s->vertices[i].normal[2] = ReadFloat(fp);
	}

	// read the index block
	s->numindicies = ReadInt(fp);
	s->indicies = (int*)Mem_Alloc(s->numindicies * sizeof(int));

	for (int i = 0; i < s->numindicies; i++)
		s->indicies[i] = ReadInt(fp);

	//CalculateNormals(s);
}

static void LoadData(FILE *fp)
{
	char header[8];
	while (ReadBytes(header, 8, fp) != 0)
	{
		//printf("processing header: %8s\n", header);
		if (!strncmp(header, "nodes", 8))
			LoadNodes(fp);
		else if (!strncmp(header, "areas", 8))
			LoadAreas(fp);
		else if (!strncmp(header, "portals", 8))
			LoadPortals(fp);
		else if (!strncmp(header, "rmodel", 8))
			LoadRenderModel(fp);
		else
			Error("Unknown header \"%8s\"\n", header);
	}
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

	viewstate.pos[0] += cmd->upmove * vectors[1][0];
	viewstate.pos[1] += cmd->upmove * vectors[1][1];
	viewstate.pos[2] += cmd->upmove * vectors[1][2];

	viewstate.pos[0] += cmd->sidemove * vectors[2][0];
	viewstate.pos[1] += cmd->sidemove * vectors[2][1];
	viewstate.pos[2] += cmd->sidemove * vectors[2][2];

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

	float	fovx, fovy;	// these are half fov in radians
	float	znear, zfar;

	// view position and view vectors
	float	pos[3];
	float	viewvectors[3][3];

	// derived values
	float	view[4][4];
	float	projection[4][4];
	float	clip[4][4];

	// clipping planes
	plane_t		planes[64];

	bool		vis;
	bool		showportals;
	int		rendermode;
	bool		shownodes;
	int		filterlevel;

} rs_t;

static rs_t	rs;

// ________________________________________________________________________________ 
// draw buffer (geometry buffer)

typedef struct drawvertex_s
{
	float	xyz[3];
	float	normal[3];
	float	color[4];

} drawvertex_t;

typedef struct drawbuffer_s
{
	int		numvertices;
	drawvertex_t	*vertices;

	int		numindicies;
	int		*indicies;

} drawbuffer_t;

static drawbuffer_t drawbuffer;

// copy the vertex data in to the drawbuffer
static void CopySurface(drawbuffer_t *b, surf_t *s)
{
	int base;

	// allocate the buffer on first call
	if (!b->vertices)
		b->vertices = (drawvertex_t*)malloc(1024 * 1024 * 2);
	if (!b->indicies)
		b->indicies = (int*)malloc(1024 * 1024 * 4);
	
	// check for overflow of the buffers
	if ((b->numvertices + s->numvertices) * sizeof(drawvertex_t) > (1024 * 1024 * 2))
		Error("Out of vertex space\n");
	if ((b->numindicies + s->numindicies) * sizeof(int) > (1024 * 1024 * 4))
		Error("Out of index space\n");

	base = b->numvertices;
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

	for (int i = 0; i < s->numindicies; i++)
	{
		b->indicies[b->numindicies] = s->indicies[i] + base;
		b->numindicies++;
	}
}

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
	for (int i = 0; i < b->numvertices; i++)
	{
		vec3 v		= Vec3FromFloat(rs.viewvectors[0]);
		vec3 n		= Vec3FromFloat(b->vertices[i].normal);
		vec3 l		= -v;
		float ndotl	= Max(Dot(n, l), 0.0f);

		// scale and bias the color slightly
		ndotl		= 0.6 * ndotl + 0.4;

		// scale the color by the 'color'
		ndotl		= 0.75f * ndotl;

		b->vertices[i].color[0] = ndotl;
		b->vertices[i].color[1] = ndotl;
		b->vertices[i].color[2] = ndotl;
		b->vertices[i].color[3] = 1.0f;
	}
}

static void DrawBuffer(drawbuffer_t *b)
{
	glVertexPointer(3, GL_FLOAT, sizeof(drawvertex_t), b->vertices->xyz);
	glNormalPointer(GL_FLOAT, sizeof(drawvertex_t), b->vertices->normal);
	glColorPointer(4, GL_FLOAT, sizeof(drawvertex_t), b->vertices->color);

	glDrawElements(GL_TRIANGLES, b->numindicies, GL_UNSIGNED_INT, (void*)b->indicies);
}

static void DrawWireframe(drawbuffer_t *b)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	// draw solid color
	glColor3f(1, 1, 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	DrawBuffer(b);

	// draw wireframe outline
	glColor3f(0, 0, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -2);
	DrawBuffer(b);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glDisableClientState(GL_VERTEX_ARRAY);
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

	// Calcuate right, left, top and bottom values
	r = rs.znear * tan(rs.fovx);
	l = -r;

	t = rs.znear * tan(rs.fovy);
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

static void SetupFOV()
{
	// calulate half x fov in radians
	float fovx = viewstate.fov * PI / 360.0f;

	// calulate the distance from projection plane to view
	float x = rs.renderwidth / tan(fovx);

	// calculate the y fov
	float fovy = atan2(rs.renderheight, x);

	rs.fovx = fovx; //* 360.0f / PI;
	rs.fovy = fovy; //* 360.0f / PI;
}

static void SetupMatrices()
{
	rs.znear	= viewstate.znear;
	rs.zfar		= viewstate.zfar;

	rs.pos[0]	= viewstate.pos[0];
	rs.pos[1]	= viewstate.pos[1];
	rs.pos[2]	= viewstate.pos[2];

	SetupViewVectors();

	SetupFOV();

	SetupModelViewMatrix();

	SetupProjectionMatrix();

	SetupGLMatrixState();

	// build the clip matrix
	MatrixMultiply(rs.clip, rs.projection, rs.view);
}

// ________________________________________________________________________________ 
// top level drawing stuff

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

static void DrawBounds(float min[3], float max[3])
{
	glBegin(GL_LINE_LOOP);
	glVertex3f(min[0], min[1], min[2]);
	glVertex3f(max[0], min[1], min[2]);
	glVertex3f(max[0], max[1], min[2]);
	glVertex3f(min[0], max[1], min[2]);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(min[0], min[1], max[2]);
	glVertex3f(max[0], min[1], max[2]);
	glVertex3f(max[0], max[1], max[2]);
	glVertex3f(min[0], max[1], max[2]);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(min[0], min[1], min[2]);
	glVertex3f(min[0], min[1], max[2]);
	glVertex3f(max[0], min[1], min[2]);
	glVertex3f(max[0], min[1], max[2]);
	glVertex3f(max[0], max[1], min[2]);
	glVertex3f(max[0], max[1], max[2]);
	glVertex3f(min[0], max[1], min[2]);
	glVertex3f(min[0], max[1], max[2]);
	glEnd();
}

static void DrawVector(float origin[3], float dir[3])
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

#if 0	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glColor3f(1, 1, 0);

#if 0
	// draw the portal vertices
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < p->numvertices; i++)
	{
		vec3 v = TransformPortalVertex(rs.clip, vec3(p->vertices[i]));

		if (v[0] < -1.0f)
			v[0] = -1.0f;
		if (v[0] > 1.0f)
			v[0] = 1.0f;
		if (v[1] < -1.0f)
			v[1] = -1.0f;
		if (v[1] > 1.0f)
			v[1] = 1.0f;

		glVertex3f(v[0], v[1], 0);
	}
	glEnd();
#endif

	// draw the portal bounding box
	box3 box = ProjectPortal(p);
	glBegin(GL_LINE_LOOP);
	glVertex3f(box.min[0], box.min[1], 0.0f);
	glVertex3f(box.max[0], box.min[1], 0.0f);
	glVertex3f(box.max[0], box.max[1], 0.0f);
	glVertex3f(box.min[0], box.max[1], 0.0f);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
#endif
}

static void DrawPortals()
{
	if (!rs.showportals)
		return;

	for (portal_t *p = portals; p; p = p->next)
		DrawPortal(p);
}

static void PresentSurfaces(drawbuffer_t *b)
{
	// flush the drawbuffer
	b->numvertices = 0;
	b->numindicies = 0;

	// build the drawbuffer from visible areas
	for (surf_t *s = surfaces; s; s = s->next)
	{
		// cull degenerate surfaces
		if (!s->numvertices || !s->numindicies)
			continue;

		CopySurface(b, s);
	}
}

static void DrawWorld()
{
	if (rs.rendermode == 0)
		DrawLit(&drawbuffer);
	else if (rs.rendermode == 1)
		DrawNormalsAsColors(&drawbuffer);
	else if (rs.rendermode == 2)
		DrawWireframe(&drawbuffer);
}

static void DrawNodesRecursive(bspnode_t *n, int level)
{
	if (!n)
		return;

	// filter levels
	if (rs.filterlevel != -1 && level > rs.filterlevel)
		return;

	DrawBounds(n->box.min, n->box.max);

	DrawNodesRecursive(n->children[0], level + 1);
	DrawNodesRecursive(n->children[1], level + 1);
}

static void DrawNodes()
{
	if (!rs.shownodes)
		return;

	glColor3f(1, 0, 0);
	glDisable(GL_DEPTH_TEST);
	DrawNodesRecursive(nodes, 0);
	glEnable(GL_DEPTH_TEST);
}

static void Draw()
{
	DrawAxis();

	DrawWorld();

	DrawPortals();

	DrawNodes();
}

static void BeginFrame()
{
	framenum++;

	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

static void EndFrame()
{}

static void DrawFrame()
{
	BeginFrame();

	SetupMatrices();

	PresentSurfaces(&drawbuffer);

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

	if (input.keys['r'])
	{
		rs.rendermode = (rs.rendermode + 1) % 3;
		printf("rendermode: %i\n", rs.rendermode);
	}

	if (input.keys['x'])
	{
		rs.vis = !rs.vis;
		printf("vis: %s\n", (rs.vis ? "on" : "off"));
	}

	if (input.keys['p'])
	{
		rs.showportals = !rs.showportals;
		printf("showportals: %i\n", (rs.showportals ? 1 : 0));
	}

	if (input.keys['n'])
	{
		rs.shownodes = !rs.shownodes;
		printf("shownodes: %i\n", (rs.shownodes ? 1 : 0));
	}

	if (input.keys['['])
	{
		rs.filterlevel = Max(-1, rs.filterlevel - 1);
		printf("filterlevel: %i\n", rs.filterlevel);
	}

	if (input.keys[']'])
	{
		rs.filterlevel += 1;
		printf("filterlevel: %i\n", rs.filterlevel);
	}
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
	rs.filterlevel = -1;
	
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("test");
	//glutHideWindow();

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

	// load the data
	FILE *fp = FileOpenBinaryRead(argv[1]);
	LoadData(fp);
	FileClose(fp);

	GLUTMain(argc, argv);

	return 0;
}

