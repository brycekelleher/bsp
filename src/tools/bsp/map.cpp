#include "vec3.h"
#include "plane.h"
#include "polygon.h"
#include "bsp.h"


// map data
static mapdata_t	mapdatalocal;
mapdata_t		*mapdata = &mapdatalocal;

static mapface_t *MallocMapPolygon(int numvertices)
{
	mapface_t *p	= (mapface_t*)MallocZeroed(sizeof(*p));
	p->polygon	= Polygon_Alloc(numvertices);

	return p;
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

	for(; count; count--)
		*f++ = ReadFloat(fp);
}

static void ReadPolygonVertex(FILE *fp, mapface_t *p)
{
	int index;
	float x, y, z;

	index	= ReadInt(fp);
	x	= ReadFloat(fp);
	y	= ReadFloat(fp);
	z	= ReadFloat(fp);

	// fixme: Debug printf
	//printf("vertex x: %f\n", x);
	//printf("vertex y: %f\n", y);
	//printf("vertex z: %f\n", z);

	// fixme: extract the polygon out
	// fixme: add vertex is useless in this case...
	p->polygon->vertices[index].x	= x * 32;
	p->polygon->vertices[index].y	= y * 32;
	p->polygon->vertices[index].z	= z * 32;
	p->polygon->numvertices++;
}

static void ReadPolygon(FILE *fp)
{
	mapface_t *p = MallocMapPolygon(32);

	while(1)
	{
		char *token = ReadToken(fp);

		if(!strcmp(token, "numvertices"))
		{
			ReadInt(fp);
		}
		else if(!strcmp(token, "vertex"))
		{
			ReadPolygonVertex(fp, p);
		}
		else if(!strcmp(token, "}"))
		{
			// fixme:
			{
				p->next = mapdata->faces;
				mapdata->faces = p;
				// These can all go under FinalizePolygon/FinishPolygon?
				// CalculateNormal()
				
				// CheckPlanar()
				
				// AddToMap()
				
				//DebugWritePolygon(p->polygon);
				DebugWriteWireFillPolygon(p->polygon);
			}
			break;
		}
	}
}

static FILE *OpenMapFile(char * filename)
{
	FILE *fp;

	fp = fopen(filename, "r");
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);

	return fp;
}

void ReadMapFile(char *filename)
{
	FILE *fp;
	char* token;

	Message("Processing %s...\n", filename);
	fp = OpenMapFile(filename);

	while((token = ReadToken(fp)))
	{
		if(!strcmp(token, "polygon"))
			ReadPolygon(fp);
	}
}

