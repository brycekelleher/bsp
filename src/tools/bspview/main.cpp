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

	for (int i = 0; i < s->numvertices; i++)
	{
		s->vertices[i].color[0] = 0.5f + 0.5f * s->vertices[i].normal[0];
		s->vertices[i].color[1] = 0.5f + 0.5f * s->vertices[i].normal[1];
		s->vertices[i].color[2] = 0.5f + 0.5f * s->vertices[i].normal[2];
		s->vertices[i].color[3] = 1.0f;
	}

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
	float	angles[2];
	float	pos[3];
	float	vectors[3][3];

	float	znear;
	float	zfar;
	float	fov;

} viewstate_t;

static viewstate_t viewstate;

typedef struct tickcmd_s
{
	float	forwardmove;
	float	sidemove;
	float	anglemove[2];

} tickcmd_t;

static tickcmd_t gcmd;

static void SetupDefaultViewState()
{
	// look down negative z
	viewstate.angles[0]	= PI / 2.0f;
	viewstate.angles[1]	= 0.0f;
	
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

	cx = 1.0f;
	sx = 0.0f;
	cy = cosf(angles[0]);
	sy = sinf(angles[0]);
	cz = cosf(angles[1]);
	sz = sinf(angles[1]);

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
	cmd->anglemove[0] = 0.0f;
	cmd->anglemove[1] = 0.0f;

	if (input.keys['w'])
		cmd->forwardmove += scale;
	if (input.keys['s'])
		cmd->forwardmove -= scale;
	if (input.keys['d'])
		cmd->sidemove += scale;
	if (input.keys['a'])
		cmd->sidemove -= scale;

	// handle mouse movement
	if(input.lbuttondown)
	{
		cmd->anglemove[0] = -0.01f * (float)input.moused[0];
		cmd->anglemove[1] = -0.01f * (float)input.moused[1];
	}
}

// apply the tick command to the viewstate
static void DoMove()
{
	tickcmd_t *cmd = &gcmd;

	VectorsFromSphericalAngles(viewstate.vectors, viewstate.angles);

	viewstate.pos[0] += cmd->forwardmove * viewstate.vectors[0][0];
	viewstate.pos[1] += cmd->forwardmove * viewstate.vectors[0][1];
	viewstate.pos[2] += cmd->forwardmove * viewstate.vectors[0][2];

	viewstate.pos[0] += cmd->sidemove * viewstate.vectors[2][0];
	viewstate.pos[1] += cmd->sidemove * viewstate.vectors[2][1];
	viewstate.pos[2] += cmd->sidemove * viewstate.vectors[2][2];

	viewstate.angles[0] += cmd->anglemove[0];
	viewstate.angles[1] += cmd->anglemove[1];

	if(viewstate.angles[1] >= PI / 2.0f)
		viewstate.angles[1] = (PI / 2.0f) - 0.001f;
	if(viewstate.angles[1] <= -PI/ 2.0f)
		viewstate.angles[1] = (-PI / 2.0f) + 0.001f;
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

static int renderwidth;
static int renderheight;

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

static void SetModelViewMatrix()
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
	matrix[0][0]	= viewstate.vectors[0][0];
	matrix[0][1]	= viewstate.vectors[0][1];
	matrix[0][2]	= viewstate.vectors[0][2];
	matrix[0][3]	= -(viewstate.vectors[0][0] * viewstate.pos[0]) - (viewstate.vectors[0][1] * viewstate.pos[1]) - (viewstate.vectors[0][2] * viewstate.pos[2]);

	matrix[1][0]	= viewstate.vectors[1][0];
	matrix[1][1]	= viewstate.vectors[1][1];
	matrix[1][2]	= viewstate.vectors[1][2];
	matrix[1][3]	= -(viewstate.vectors[1][0] * viewstate.pos[0]) - (viewstate.vectors[1][1] * viewstate.pos[1]) - (viewstate.vectors[1][2] * viewstate.pos[2]);

	matrix[2][0]	= viewstate.vectors[2][0];
	matrix[2][1]	= viewstate.vectors[2][1];
	matrix[2][2]	= viewstate.vectors[2][2];
	matrix[2][3]	= -(viewstate.vectors[2][0] * viewstate.pos[0]) - (viewstate.vectors[2][1] * viewstate.pos[1]) - (viewstate.vectors[2][2] * viewstate.pos[2]);

	matrix[3][0]	= 0.0f;
	matrix[3][1]	= 0.0f;
	matrix[3][2]	= 0.0f;
	matrix[3][3]	= 1.0f;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GL_MultMatrixTranspose(yrotate);
	GL_MultMatrixTranspose(matrix);
}

static void SetPerspectiveMatrix()
{
	float r, l, t, b;
	float fovx, fovy;
	float m[4][4];

	fovx = viewstate.fov * (PI / 360.0f);
	float x = (renderwidth / 2.0f) / atan(fovx);
	fovy = atan2(renderheight / 2.0f, x);

	// Calcuate right, left, top and bottom values
	r = viewstate.znear * fovx;
	l = -r;

	t = viewstate.znear * fovy;
	b = -t;

	m[0][0] = (2.0f * viewstate.znear) / (r - l);
	m[1][0] = 0;
	m[2][0] = (r + l) / (r - l);
	m[3][0] = 0;

	m[0][1] = 0;
	m[1][1] = (2.0f * viewstate.znear) / (t - b);
	m[2][1] = (t + b) / (t - b);
	m[3][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = -(viewstate.zfar + viewstate.znear) / (viewstate.zfar - viewstate.znear);
	m[3][2] = -2.0f * viewstate.zfar * viewstate.znear / (viewstate.zfar - viewstate.znear);

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = -1;
	m[3][3] = 0;

	glMatrixMode(GL_PROJECTION);
	GL_LoadMatrix(m);
}

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

static float Max(float a, float b)
{
	return (a > b ? a : b);
}

static float Min(float a, float b)
{
	return (a < b ? a : b);
}

static void CalculateLighting(surf_t *s)
{
	for (int i = 0; i < s->numvertices; i += 3)
	{
		vec3 normal	= Vec3FromFloat(s->vertices[i].normal);
		vec3 view	= Vec3FromFloat(viewstate.vectors[0]);
		vec3 l		= -view;
		float ndotl	= Max(Dot(normal, l), 0.0f);

		// scale and bias the color slightly
		ndotl		= 0.6 * ndotl + 0.4;

		for (int j = 0; j < 3; j++)
		{
			s->vertices[i + j].color[0] = ndotl;
			s->vertices[i + j].color[1] = ndotl;
			s->vertices[i + j].color[2] = ndotl;
			//s->vertices[i + j].color[0] = 0.5f + 0.5f * normal[0];
			//s->vertices[i + j].color[1] = 0.5f + 0.5f * normal[1];
			//s->vertices[i + j].color[2] = 0.5f + 0.5f * normal[2];
			s->vertices[i + j].color[3] = 1.0f;
		}
	}
}

static void DrawSurface(surf_t *s)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	CalculateLighting(s);

	glVertexPointer(3, GL_FLOAT, sizeof(surfvertex_t), s->vertices->xyz);
	glColorPointer(4, GL_FLOAT, sizeof(surfvertex_t), s->vertices->color);

	glDrawArrays(GL_TRIANGLES, 0, s->numvertices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void DrawSurfaces()
{
	for (int i = 0; i < numareas; i++)
	{
		area_t *a = areas + i;

		if (!a->surfaces)
			continue;

		DrawSurface(a->surfaces);
	}
}

static void DrawSurfacesWireframe()
{
	static float white[] = { 1, 1, 1 };
	static float black[] = { 0, 0, 0 };

	glEnableClientState(GL_VERTEX_ARRAY);

	// draw solid color
	glColor3fv(white);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	DrawSurfaces();

	// draw wireframe outline
	glColor3fv(black);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -2);
	DrawSurfaces();
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisableClientState(GL_VERTEX_ARRAY);
}

static void Draw()
{
	DrawAxis();

	DrawSurfaces();

	DrawPortals();
}

static void BeginFrame()
{
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	SetModelViewMatrix();

	SetPerspectiveMatrix();

	glViewport(0, 0, renderwidth, renderheight);
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
	renderwidth = w;
	renderheight = h;
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

