#include "bsp.h"

static FILE *debugfp;

// fixme: move this into another file
static FILE *OpenFile(const char *filename)
{
	FILE *fp;
	
	fp = fopen(filename, "w");
	if(!fp)
		Error("Failed to open file \"%s\"\n", filename);
	
	return fp;
}

void DebugWritePolygon(polygon_t *p)
{
	fprintf(debugfp, "color 1 0 0 1\n");
	fprintf(debugfp, "polygon\n");
	fprintf(debugfp, "%i\n", p->numvertices);
	for(int i = 0; i < p->numvertices; i++)
		fprintf(debugfp, "%f %f %f\n",
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
}

void DebugWriteWireFillPolygon(polygon_t *p)
{
	fprintf(debugfp, "color 1 0 0 1\n");
	fprintf(debugfp, "polyline\n");
	fprintf(debugfp, "%i\n", p->numvertices + 1);
	for(int i = 0; i < p->numvertices; i++)
		fprintf(debugfp, "%f %f %f\n",
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
	
	fprintf(debugfp, "%f %f %f\n",
		p->vertices[0][0],
		p->vertices[0][1],
		p->vertices[0][2]);
}

void DebugInit()
{
	debugfp = OpenFile("debug.txt");
}

void DebugShutdown()
{
	fclose(debugfp);
}
