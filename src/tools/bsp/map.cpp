#include <ctype.h>
#include "bsp.h"

// map data
static mapdata_t	mapdatalocal;
mapdata_t		*mapdata = &mapdatalocal;

smodel_t *smodels = NULL;

static mapface_t *MallocMapPolygon(polygon_t *p)
{
	mapface_t *face	= (mapface_t*)MallocZeroed(sizeof(*face));

	return face;
}

static int linenum;
static int polygonlinenum;
static char buffer[1024];

static char *ReadToken(FILE *fp)
{
	int c;

	c = fgetc(fp);
	for (; c != EOF && isspace(c); c = fgetc(fp))
		if (c == '\n')
			linenum++;

	int i = 0;
	for (; c != EOF && !isspace(c); c = fgetc(fp))
		buffer[i++] = c;

	buffer[i] = '\0';

	// unget the previous character that caused the break from the loop
	if (c != EOF)
		ungetc(c, fp);

	if (i == 0)
		return NULL;

	return buffer;
}

static void ExpectToken(char *expect, FILE *fp)
{
	char *token = ReadToken(fp);

	if (token && strcmp(expect, token))
		Error("Expected token \"%s\" read \"%s\"\n", expect, token);
}

static char* PolygonString(polygon_t *p)
{
	static char buffer[2048];
	char *s = buffer;

	s += sprintf(s, "polygon\n");

	for(int i = 0; i < p->numvertices; i++)
	{
		s += sprintf(s, "%i (%f %f %f)\n",
			i,
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
	}

	return buffer;
}

static void CheckSize(polygon_t *p)
{
	for (int i = 0; i < p->numvertices; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if ((p->vertices[i][j] > MAX_VERTEX_SIZE) || (p->vertices[i][j] < -MAX_VERTEX_SIZE))
				Error("(%i) Polygon is larger than max size\n%s", polygonlinenum, PolygonString(p));
		}
	}
}

static void CheckDegenerate(polygon_t *p)
{
	if (Polygon_Area(p) < AREA_EPSILON)
		Error("(%i) Polygon has degenerate area\n%s", polygonlinenum, PolygonString(p));
}

static void CheckPlanar(polygon_t *p)
{
	plane_t plane;

	plane = Polygon_Plane(p);

	for (int i = 0; i < p->numvertices; i++)
		if (PointOnPlaneSide(plane, p->vertices[i], PLANAR_EPSILON) != PLANE_SIDE_ON)
			Error("(%i) Polygon is non-planar\n%s", polygonlinenum, PolygonString(p));
}

static void ValidatePolygon(polygon_t *p)
{
	CheckSize(p);

	CheckDegenerate(p);

	CheckPlanar(p);
}

static void FinalizePolygon()
{
	// CalculateNormal()
	
	// CheckPlanar()
	
	// AddToMap()
}

static int ReadInt(FILE *fp)
{
	int i = atoi(ReadToken(fp));
	return i;
}

static float ReadFloat(FILE *fp)
{
	float f = (float)atof(ReadToken(fp));
	return f;
}

static void ReadFloatList(FILE *fp, float *floatlist, int count)
{
	float *f = floatlist;

	for (; count; count--)
		*f++ = ReadFloat(fp);
}

static void ReadPolygonVertex(FILE *fp, polygon_t *p)
{
	int index;
	float x, y, z;

	index	= ReadInt(fp);
	x	= ReadFloat(fp);
	y	= ReadFloat(fp);
	z	= ReadFloat(fp);

	p->vertices[index].x	= x;
	p->vertices[index].y	= y;
	p->vertices[index].z	= z;
	p->numvertices++;
}

static polygon_t *ReadPolygon(FILE *fp)
{
	polygon_t *p = NULL;
	
	polygonlinenum = linenum;

	ExpectToken("{", fp);

	while (1)
	{
		char *token = ReadToken(fp);

		if (!strcmp(token, "numvertices"))
		{
			int numvertices = ReadInt(fp);
			p = Polygon_Alloc(numvertices);
		}
		else if (!strcmp(token, "vertex"))
		{
			if (!p)
				p = Polygon_Alloc(32);

			ReadPolygonVertex(fp, p);
		}
		else if (!strcmp(token, "}"))
		{
			ValidatePolygon(p);

			return p;
		}
		else
		{
			Error("(%i) Unknown token \"%s\" when reading polygon\n", linenum, token);
		}
	}
}

static void ReadMapFace(FILE *fp)
{
	polygon_t *p = ReadPolygon(fp);
	
	mapface_t *face = MallocMapPolygon(p);
	face->polygon	= p;
	face->plane	= Polygon_Plane(p);
	face->box	= Polygon_BoundingBox(p);
	face->areahint	= false;
	
	// link the face into the map list
	face->next = mapdata->faces;
	mapdata->faces = face;
	
	mapdata->numfaces++;
	
	static float white[3] = { 1, 1, 1 };
	DebugWriteColor(debugfp, white);
	DebugWriteWireFillPolygon(debugfp, face->polygon);
}

static void ReadAreaHint(FILE *fp)
{
	polygon_t *p = ReadPolygon(fp);
	
	mapface_t *face = MallocMapPolygon(p);
	face->polygon	= p;
	face->plane	= Polygon_Plane(p);
	face->box	= Polygon_BoundingBox(p);
	face->areahint	= true;

	// link the face into the map list
	face->next = mapdata->faces;
	mapdata->faces = face;
	
	mapdata->numareahints++;

	static float green[3] = { 0, 1, 0 };
	DebugWriteColor(debugfp, green);
	DebugWriteWireFillPolygon(debugfp, face->polygon);
}

static void ReadStaticModel(FILE *fp)
{
	smodel_t *m = (smodel_t*)MallocZeroed(sizeof(smodel_t));
	
	ExpectToken("{", fp);

	while (1)
	{
		char *token = ReadToken(fp);

		if (!strcmp(token, "numvertices"))
		{
			m->numvertices = ReadInt(fp);
			m->vertices = (vec3*)MallocZeroed(m->numvertices * sizeof(vec3));
		}
		else if (!strcmp(token, "vertex"))
		{
			int index = ReadInt(fp);
			m->vertices[index].x = ReadFloat(fp);
			m->vertices[index].y = ReadFloat(fp);
			m->vertices[index].z = ReadFloat(fp);
			m->numvertices++;
		}
		else if (!strcmp(token, "numtris"))
		{
			m->numindicies = 3 * ReadInt(fp);
			m->indicies = (int*)MallocZeroed(m->numindicies * sizeof(int));
		}
		else if (!strcmp(token, "tri"))
		{
			int index = ReadInt(fp);
			m->indicies[3 * index + 0] = ReadInt(fp);
			m->indicies[3 * index + 1] = ReadInt(fp);
			m->indicies[3 * index + 2] = ReadInt(fp);
		}
		else if (!strcmp(token, "}"))
		{
			// link it into the static list
			m->next = smodels;
			smodels = m;
			return;
		}
		else
		{
			Error("(%i) Unknown token \"%s\" when reading static model\n", linenum, token);
		}
	}
}

static void ReadMapFile(FILE *fp)
{
	char *token;
	
	while ((token = ReadToken(fp)) != NULL)
	{
		if (!strcmp(token, "polygon"))
		{
			ReadMapFace(fp);
		}
		else if (!strcmp(token, "areahint"))
		{
			ReadAreaHint(fp);
		}
		else if (!strcmp(token, "staticmodel"))
		{
			ReadStaticModel(fp);
		}
		else
		{
			Error("(%i) Unknown token \"%s\" when reading map\n", linenum, token);
		}
	}
	
	Message("%i faces\n", mapdata->numfaces);
	Message("%i areahints\n", mapdata->numareahints);
}

void ReadMap(char *filename)
{
	FILE *fp;

	Message("Reading map \"%s\"\n", filename);
	
	fp = FileOpenBinaryRead(filename);

	ReadMapFile(fp);

	FileClose(fp);
}

