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

void DebugWriteColor(FILE *fp, float *rgb)
{
	fprintf(fp, "color %f %f %f 1\n", rgb[0], rgb[1], rgb[2]);
}

void DebugInit()
{
	debugfp = FileOpenTextWrite("debug.gld");
}

void DebugShutdown()
{
	fclose(debugfp);
}

vec3 Center(box3 box)
{
	return 0.5f * (box.min + box.max);
}

static void HSVToRGB(float rgb[3], float h, float s, float v)
{
	float r, g, b;
	
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
	switch(i % 6)
	{
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}
	
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void DebugWritePortalFile(bsptree_t *tree)
{
	FILE *fp = FileOpenTextWrite("portal_debug.gld");
	
	// iterate through all leafs
	int leafnum = 0;
	for (bspnode_t *l = tree->leafs; l; l = l->leafnext, leafnum++)
	{
		// skip leaves which aren't empty
		if(!l->empty)
			continue;
		
		// calculate a color for the portal polygon
		{
			//fprintf(fp, "color 1 0 0 1\n");
			float rgb[3];
			float f = (float)leafnum / tree->numleafs;
			f = 4.0f * f;
			f = f - floor(f);
			HSVToRGB(rgb, f, 1.0f, 1.0f);
			fprintf(fp, "color %f %f %f 1\n", rgb[0], rgb[1], rgb[2]);
		}

		for (portal_t *p = l->portals; p; p = p->leafnext)
		{
			fprintf(fp, "polyline\n");
			fprintf(fp, "%i\n", p->polygon->numvertices + 1);
			
			for(int i = 0; i < p->polygon->numvertices + 1; i++)
			{
				vec3 v = p->polygon->vertices[i % p->polygon->numvertices];

				// scale the vertices so the indvidual portals are visible
#if 1
				{
					vec3 center = l->box.Center();
					vec3 boxsize =  l->box.Size();

					vec3 scale;
					scale[0] = (boxsize[0] - 8.0f) / boxsize[0];
					scale[1] = (boxsize[1] - 8.0f) / boxsize[1];
					scale[2] = (boxsize[2] - 8.0f) / boxsize[2];

					v[0] = center[0] + (scale[0] * ( v[0] - center[0]));
					v[1] = center[1] + (scale[1] * ( v[1] - center[1]));
					v[2] = center[2] + (scale[2] * ( v[2] - center[2]));
				}
#endif
				
				// write the vertex data
				fprintf(fp, "%f %f %f\n",
					v[0],
					v[1],
					v[2]);
			}
		}
	}
	
	fclose(fp);
}

static int leafnum = 0;
void DebugWritePortalPolygon(bsptree_t *tree, polygon_t *p)
{
	static FILE *fp = NULL;

	//if (leafnum != 1)
	//	return;

	if (!fp)
		fp = FileOpenTextWrite("portal_src.gld");

	// write the color
	float rgb[3];
	HSVToRGB(rgb, (float)leafnum / tree->numleafs, 1.0f, 1.0f);
	fprintf(fp, "color %f %f %f 1\n", rgb[0], rgb[1], rgb[2]);

	DebugWriteWireFillPolygon(fp, p);
	fflush(fp);
}

void DebugEndLeafPolygons()
{
	leafnum++;
}

