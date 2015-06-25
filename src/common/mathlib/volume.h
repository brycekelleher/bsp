#ifndef __VOLUME_H__
#define __VOLUME_H__

#include "polygon.h"

typedef struct volume_side_s
{
	polygon_t	*polygon;
} volume_side_t;

typedef struct volume_s
{
	int	maxsides;
	int	numsides;
	volume_side_t	*sides;
} volume_t;

volume_t *Volume_Alloc(int maxsides);
void Volume_Free(volume_t *v);
volume_t *Volume_Copy(volume_t *v);
volume_t *Volume_Reverse(volume_t *v);
void Volume_SplitWithPlane(volume_t* v, vec3 normal, float dist, float epsilon, volume_t **f, volume_t **b);
void Volume_OnPlaneSide(volume_t *v, vec3 normal, float dist);

#endif
