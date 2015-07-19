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

vec3 Center(box3 box)
{
	return 0.5f * (box.min + box.max);
}

void DebugWritePortalFile(bsptree_t *tree)
{
	FILE *fp = FileOpenTextWrite("portal_debug.gld");
	
	// iterate through all leafs
	for (bspnode_t *l = tree->leafs; l; l = l->leafnext)
	{
		for (portal_t *p = l->portals; p; p = p->leafnext)
		{
			//DebugWriteWireFillPolygon(fp, p->polygon);
			vec3 center = Center(l->box);
			
			fprintf(fp, "color 1 0 0 1\n");
			fprintf(fp, "polyline\n");
			fprintf(fp, "%i\n", p->polygon->numvertices + 1);
			
			for(int i = 0; i < p->polygon->numvertices + 1; i++)
			{
				vec3 v =
				center + (0.95f * (p->polygon->vertices[i % p->polygon->numvertices] - center));
		
				
				fprintf(fp, "%f %f %f\n",
					v[0],
					v[1],
					v[2]);
			}
		}
	}
	
	fclose(fp);
	// write portal data to file
}

