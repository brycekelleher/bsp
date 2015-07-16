#include "bsp.h"

// global debug file pointer
FILE *debugfp = NULL;

void DebugPrintfPolygon(FILE *fp, polygon_t *p)
{
	fprintf(stdout, "polygon %p, numvertices=%i, maxvertices=%i\n", p, p->numvertices, p->maxvertices);
	for(int i = 0; i < p->numvertices; i++)
		fprintf(fp, "vertex %i: (%f %f %f)\n",
			i,
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
}

void DebugWritePolygon(FILE *fp, polygon_t *p)
{
	fprintf(fp, "color 1 0 0 1\n");
	fprintf(fp, "polygon\n");
	fprintf(fp, "%i\n", p->numvertices);
	for(int i = 0; i < p->numvertices; i++)
		fprintf(fp, "%f %f %f\n",
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
}

void DebugWriteWireFillPolygon(FILE *fp, polygon_t *p)
{
	fprintf(fp, "color 1 0 0 1\n");
	fprintf(fp, "polyline\n");
	fprintf(fp, "%i\n", p->numvertices + 1);
	for(int i = 0; i < p->numvertices; i++)
		fprintf(fp, "%f %f %f\n",
			p->vertices[i][0],
			p->vertices[i][1],
			p->vertices[i][2]);
	
	fprintf(fp, "%f %f %f\n",
		p->vertices[0][0],
		p->vertices[0][1],
		p->vertices[0][2]);
}

void DebugInit()
{
	debugfp = FileOpenTextWrite("debug.txt");
}

void DebugShutdown()
{
	fclose(debugfp);
}
