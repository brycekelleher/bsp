#include <stdlib.h>
#include <memory.h>
#include "volume.h"

int Volume_NumBytes(int maxsides)
{
	return sizeof(sizeof(volume_t) + maxsides * sizeof(volume_side_t));
}

volume_t *Volume_Alloc(int maxsides)
{
	volume_t	*v;

	int numbytes = Volume_NumBytes(maxsides);
	v = (volume_t*)malloc(numbytes);

	v->maxsides	= maxsides;
	v->numsides	= 0;
	v->sides	= (volume_side_t*)(v + 1);

	return v;
}

void Volume_Free(volume_t *v)
{
	free(v);
}

volume_t *Volume_Copy(volume_t *v)
{
	volume_t	*c;

	int numbytes = Volume_NumBytes(v->maxsides);
	c = (volume_t*)malloc(numbytes);

	memcpy(c, v, numbytes);

	for(int i = 0; i < v->numsides; i++)
	{
		c->sides[i].polygon = Polygon_Copy(v->sides[i].polygon);
	}

	return c;
}

volume_t *Volume_Reverse(volume_t *v)
{
	volume_t* r = Volume_Alloc(v->maxsides);

	for(int i = 0; i < v->numsides; i++)
	{
		// copy the sides and reverse the polygon
		r->sides[i] = v->sides[i];

		r->sides[i].polygon = Polygon_Reverse(v->sides[i].polygon);
	}

	return r;
}

