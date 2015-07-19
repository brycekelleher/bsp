#include "vec3.h"
#include "plane.h"
#include "polygon.h"
#include "bsp.h"


// map data
static mapdata_t	mapdatalocal;
mapdata_t		*mapdata = &mapdatalocal;

static mapface_t *MallocMapPolygon(polygon_t *p)
{
	mapface_t *face	= (mapface_t*)MallocZeroed(sizeof(*face));

	return face;
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

	// fixme: extract the polygon out
	// fixme: add vertex is useless in this case...
	p->vertices[index].x	= x;
	p->vertices[index].y	= y;
	p->vertices[index].z	= z;
	p->numvertices++;
}

static polygon_t *ReadPolygon(FILE *fp)
{
	polygon_t *p = NULL;
	
	while (1)
	{
		char *token = ReadToken(fp);

		if (!strcmp(token, "numvertices"))
		{
			int numvertices = ReadInt(fp);
			p = Polygon_Alloc(numvertices);

			ReadInt(fp);
		}
		else if (!strcmp(token, "vertex"))
		{
			if (!p)
				p = Polygon_Alloc(32);

			ReadPolygonVertex(fp, p);
		}
		else if (!strcmp(token, "{"))
		{
			continue;
		}
		else if (!strcmp(token, "}"))
		{
			return p;
		}
		else
		{
			Error("Uknown token \"%s\" when reading polygon\n", token);
		}
	}
}

static void ReadMapFace(FILE *fp)
{
	// These can all go under FinalizePolygon/FinishPolygon?
	// CalculateNormal()
	
	// CheckPlanar()
	
	// AddToMap()
	
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
}

void ReadMapFile(FILE *fp)
{
	char *token;
	
	while ((token = ReadToken(fp)))
	{
		if (!strcmp(token, "polygon"))
		{
			ReadMapFace(fp);
		}
		else if (!strcmp(token, "areahint"))
		{
			ReadAreaHint(fp);
		}
		else
		{
			Error("Unknown token \"%s\"\n", token);
		}
	}
	
	Message("%i faces\n", mapdata->numfaces);
	Message("%i areahints\n", mapdata->numareahints);
}

void ReadMap(char *filename)
{
	FILE *fp;

	Message("Reading map \"%s\"\n", filename);
	
	fp = FileOpenTextRead(filename);

	ReadMapFile(fp);
}

