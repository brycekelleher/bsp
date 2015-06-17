#include "bsp.h"


static void Walk(bspnode_t *n)
{
	if(!n)
	{
		return;
	}

	if(n->children[0])
		Walk(n->children[0]);
	if(n->children[1])
		Walk(n->children[1]);
}
