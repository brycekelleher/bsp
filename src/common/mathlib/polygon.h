#ifndef __POLYGON_H__
#define __POLYGON_H__

//#include "vec3.h"
//#include "plane.h"

class vec3;
class box3;
class plane_t;

typedef struct polygon_s
{
	int	maxvertices;
	int	numvertices;
	vec3	*vertices;

} polygon_t;

void Polygon_SetMemCallbacks(void *(*alloccallback)(int numbytes), void (*freecallback)(void *p));

// allocates a new polygon with numvertices
polygon_t *Polygon_Alloc(int numvertices);

// frees the polygon
void Polygon_Free(polygon_t* p);

// creates a copy of the polygon
polygon_t* Polygon_Copy(polygon_t* p);

// adds a vertex to the polygon
void Polygon_AddVertex(float x, float y, float z);

// reverses the winding order of the polygon
polygon_t *Polygon_Reverse(polygon_t* p);

// returns the polygon bounding box
box3 Polygon_BoundingBox(polygon_t* p);

// returns the centroid of the polygon
vec3 Polygon_Centroid(polygon_t* p);

// returns the polygon area vector
// each component is the projected area in x, y, z
// taking the length returns the actual area of the polygon
vec3 Polygon_AreaVector(polygon_t *p);

// returns the area of the polygon
float Polygon_Area(polygon_t *p);

vec3 Polygon_Normal(polygon_t* p);

// returns the plane that the polygon lies in
plane_t Polygon_Plane(polygon_t *p);

// split the polygon with plane returning the front and back pieces if they exist
void Polygon_SplitWithPlane(polygon_t *in, plane_t plane, float epsilon, polygon_t **front, polygon_t **back);

// clip the polygon with the plane returning the front piece if it exists
polygon_t *Polygon_ClipWithPlane(polygon_t *p, plane_t plane, float epsilon);

// return which side of the plane the polygon is on
int Polygon_OnPlaneSide(polygon_t *p, plane_t plane, float epsilon);

#endif

