#include "bsp.h"

static void EmitBytes(void *data, int numbytes, FILE *fp)
{
	fwrite(data, numbytes, 1, fp);
}

static void EmitInt(int i, FILE* fp)
{
	EmitBytes(&i, sizeof(int), fp);
}

static void EmitFloat(float f, FILE *fp)
{
	EmitBytes(&f, sizeof(float), fp);
}

// emit nodes in postorder (leaves at start of file)
static int numnodes = 0;
static int EmitNode(bspnode_t *n, FILE *fp)
{
	// termination guard
	if(!n)
	{
		return -1;
	}
	
	// emit the children
	int childnum[2];
	childnum[0] = EmitNode(n->children[0], fp);
	childnum[1] = EmitNode(n->children[1], fp);
	
	// write the node data
	{
		EmitInt(childnum[0], fp);
		EmitInt(childnum[1], fp);

// disable this to check the node linkage
#if 1
		// Emit the plane data
		EmitFloat(n->plane.a, fp);
		EmitFloat(n->plane.b, fp);
		EmitFloat(n->plane.c, fp);
		EmitFloat(n->plane.d, fp);
#endif
	}
	
	int thisnode = numnodes++;
	return thisnode;
}

static void EmitTree(bsptree_t *tree, FILE *fp)
{
	EmitInt(tree->numnodes, fp);
	EmitInt(tree->numleafs, fp);
	
	int rootnode = EmitNode(tree->root, fp);
	
}

void WriteBinary(bsptree_t *tree)
{
	Message("Writing binary \"%s\"...\n", outputfilename);
	
	FILE *fp = FileOpenBinaryWrite(outputfilename);
	
	EmitTree(tree, fp);
}
