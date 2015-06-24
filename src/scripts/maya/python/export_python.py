import maya.OpenMaya as OM

# this won't work as the iterator will hit meshes more than once if they are instanced
# we want to visit each mesh node once and only once

def IsNodeType(node, nodetype):
	return (obj.apiType() == nodetype)

def GetAllPathsForNode(node):
	patharray = OM.MDagPathArray()
	OM.MDagPath.getAllPathsTo(node, patharray)    
	return patharray

def GetTransformMatrix(node):
	return OM.MFnTransform(node).transformation().asMatrix()

def EmitPolygon(vertices, indicies, transform):
	print 'polygon'
	print '{'
	for i in range(indicies.length()):
		v = vertices[indicies[i]] * transform
		print 'vertex %i %f %f %f' % (i, v[0], v[1], v[2])
	print '}'

def EmitMesh(node, transform):
	mesh = OM.MFnMesh(node)

	# get the mesh vertices in object space
	vertices = OM.MPointArray()
	mesh.getPoints(vertices, OM.MSpace.kObject);

	# iterate through the polygons
	for i in range(mesh.numPolygons()):
		indicies = OM.MIntArray()
		mesh.getPolygonVertices(i, indicies)
		EmitPolygon(vertices, indicies, transform)

# grab all the paths to this node
# for each path emit the geometry
def EmitMeshWithAllPaths(obj):
	patharray = GetAllPathsForNode(obj)

	for x in range(patharray.length()):
		# get the transform for this path
		transform = GetNodeTransformMatrix(patharray[x].transform());

		# emit the mesh with this transform
		EmitMesh(obj, transform)

def IterateNodes():
	it = OM.MItDag()
	while(not it.isDone()):
		obj = it.currentItem();

		if (obj.apiType() == OM.MFn.kMesh):
			EmitMeshWithAllPaths(obj)

		it.next()

def Main():
	OM.MGlobal.displayInfo('iterating nodes')
	IterateNodes()

Main()
