import maya.OpenMaya as OM

def EnumeratePaths(obj):
	node = OM.MFnDagNode(obj)

	patharray = OM.MDagPathArray()
	node.getAllPaths(patharray)

	OM.MDagPath.getAllPathsTo(obj, patharray)    

	for x in range(patharray.length()):
		print 'path: ' + patharray[x].fullPathName()
		print 'lowest transform: ' + OM.MFnDagNode(patharray[x].transform()).name()
		
		# this line extracts the transformation matrix
	        matrix = OM.MFnTransform(patharray[x].transform()).transformation().asMatrix()
	        vector = OM.MVector(1, 1, 1)
	        vector * matrix

def EnumerateNodeChildren(obj):
	node = OM.MFnDagNode(obj)

	for x in range(node.childCount()):
		child = OM.MFnDagNode(node.child(x))
		print 'child: ' + child.name()

def EmitMesh(node):
	mesh = OM.MFnMesh(node)

	print 'numpolygons: ' + str(mesh.numPolygons())

	vertices = OM.MPointArray()
	mesh.getPoints(vertices, OM.MSpace.kObject);

	for x in range(vertices.length()):
		print 'vertices: ' + str(vertices[x][0]) + ' ' + str(vertices[x][1]) + ' ' + str(vertices[x][2])

	indicies = OM.MIntArray()
	mesh.getPolygonVertices(0, indicies)

	#could use mesh.polygonVertexCount
	for x in range(indicies.length()):
		print 'indicies: ' + str(indicies[x])

def IterateNodes():
	it = OM.MItDag()
	while(not it.isDone()):
		obj = it.currentItem();

		print 'node: ' + obj.apiTypeStr()

		if (obj.apiType() == OM.MFn.kMesh):
			print 'found mesh'
			EmitMesh(obj)

		it.next()

def Main():
	OM.MGlobal.displayInfo('iterating nodes')
	IterateNodes()

Main()
